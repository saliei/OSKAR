/*
 * Copyright (c) 2012, The University of Oxford
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the University of Oxford nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <cuda_runtime_api.h>
#include <omp.h>

#include "apps/lib/oskar_settings_load.h"
#include "apps/lib/oskar_set_up_sky.h"
#include "apps/lib/oskar_set_up_telescope.h"
#include "apps/lib/oskar_set_up_visibilities.h"
#include "apps/lib/oskar_sim_interferometer.h"
#include "apps/lib/oskar_visibilities_write_ms.h"
#include "interferometry/oskar_evaluate_uvw_baseline.h"
#include "interferometry/oskar_interferometer.h"
#include "interferometry/oskar_TelescopeModel.h"
#include "interferometry/oskar_SettingsTelescope.h"
#include "interferometry/oskar_Visibilities.h"
#include "interferometry/oskar_visibilities_get_channel_amps.h"
#include "interferometry/oskar_visibilities_write.h"
#include "interferometry/oskar_visibilities_add_system_noise.h"
#include "sky/oskar_SkyModel.h"
#include "sky/oskar_SettingsSky.h"
#include "sky/oskar_sky_model_free.h"
#include "utility/oskar_log_error.h"
#include "utility/oskar_log_message.h"
#include "utility/oskar_log_section.h"
#include "utility/oskar_log_settings.h"
#include "utility/oskar_log_warning.h"
#include "utility/oskar_Log.h"
#include "utility/oskar_Mem.h"
#include "utility/oskar_mem_clear_contents.h"
#include "utility/oskar_mem_init.h"
#include "utility/oskar_mem_free.h"
#include "utility/oskar_mem_add.h"
#include "utility/oskar_Settings.h"
#include "utility/oskar_settings_free.h"
#include "imaging/oskar_make_image.h"
#include "imaging/oskar_image_write.h"
#ifndef OSKAR_NO_FITS
#include "fits/oskar_fits_image_write.h"
#endif

#include <QtCore/QTime>

#include <cstdlib>
#include <cmath>
#include <vector>

using std::vector;

extern "C"
int oskar_sim_interferometer(const char* settings_file, oskar_Log* log)
{
    int error;

    // Load the settings file.
    oskar_Settings settings;
    oskar_log_section(log, "Loading settings file '%s'", settings_file);
    error = oskar_settings_load(&settings, log, settings_file);
    if (error) return error;
    int type = settings.sim.double_precision ? OSKAR_DOUBLE : OSKAR_SINGLE;

    // Log the relevant settings.
    log->keep_file = settings.sim.keep_log_file;
    oskar_log_settings_simulator(log, &settings);
    oskar_log_settings_sky(log, &settings);
    oskar_log_settings_observation(log, &settings);
    oskar_log_settings_telescope(log, &settings);
    oskar_log_settings_interferometer(log, &settings);
    if (settings.interferometer.image_interferometer_output)
        oskar_log_settings_image(log, &settings);

    // Check that a data file has been specified.
    if ( !(settings.interferometer.oskar_vis_filename ||
            settings.interferometer.ms_filename ||
            (settings.interferometer.image_interferometer_output &&
                    (settings.image.oskar_image || settings.image.fits_image))))
    {
        oskar_log_error(log, "No output file specified.");
        return OSKAR_ERR_SETTINGS;
    }

    // Find out how many GPUs we have.
    int device_count = 0;
    int num_devices = settings.sim.num_cuda_devices;
    error = (int)cudaGetDeviceCount(&device_count);
    if (error) return error;
    if (device_count < num_devices) return OSKAR_ERR_CUDA_DEVICES;

    // Set up the telescope model.
    oskar_TelescopeModel tel_cpu;
    error = oskar_set_up_telescope(&tel_cpu, log, &settings);
    if (error) return OSKAR_ERR_SETUP_FAIL_TELESCOPE;

    // Set up the sky model array.
    oskar_SkyModel* sky_chunk_cpu = NULL;
    int num_sky_chunks = 0;
    error = oskar_set_up_sky(&num_sky_chunks, &sky_chunk_cpu, log, &settings);

    // Create the global visibility structure on the CPU.
    int complex_matrix = type | OSKAR_COMPLEX | OSKAR_MATRIX;
    oskar_Visibilities vis_global;
    oskar_set_up_visibilities(&vis_global, &settings, &tel_cpu, complex_matrix,
            &error);

    // Create temporary and accumulation buffers to hold visibility amplitudes
    // (one per thread/GPU).
    // These are held in standard vectors so that the memory will be released
    // automatically if the function returns early, or is terminated.
    vector<oskar_Mem> vis_acc(num_devices), vis_temp(num_devices);
    int time_baseline = tel_cpu.num_baselines() * settings.obs.num_time_steps;
    for (int i = 0; i < num_devices; ++i)
    {
        oskar_mem_init(&vis_acc[i], complex_matrix, OSKAR_LOCATION_CPU,
                time_baseline, true, &error);
        oskar_mem_init(&vis_temp[i], complex_matrix, OSKAR_LOCATION_CPU,
                time_baseline, true, &error);
        if (error) return error;
        error = cudaSetDevice(settings.sim.cuda_device_ids[i]);
        if (error) return error;
        cudaDeviceSynchronize();
    }

    // Set the number of host threads to use (one per GPU).
    omp_set_num_threads(num_devices);

    // Run the simulation.
    oskar_log_section(log, "Starting simulation...");
    QTime timer;
    timer.start();
    for (int c = 0; c < settings.obs.num_channels; ++c)
    {
        double frequency;
        oskar_Mem vis_amp;

        frequency = settings.obs.start_frequency_hz +
                c * settings.obs.frequency_inc_hz;
        oskar_log_message(log, 0, "Channel %3d/%d [%.4f MHz]",
                c + 1, settings.obs.num_channels, frequency / 1e6);

        // Use OpenMP dynamic scheduling for loop over chunks.
#pragma omp parallel for schedule(dynamic, 1)
        for (int i = 0; i < num_sky_chunks; ++i)
        {
            if (error) continue;

            // Get thread ID for this chunk.
            int thread_id = omp_get_thread_num();

            // Get device ID and device properties for this chunk.
            int device_id = settings.sim.cuda_device_ids[thread_id];

            // Set the device to use for the chunk.
            error = cudaSetDevice(device_id);

            // Run simulation for this chunk.
            oskar_interferometer(&(vis_temp[thread_id]), log,
                    &(sky_chunk_cpu[i]), &tel_cpu, &settings, frequency,
                    i, num_sky_chunks, &error);

            oskar_mem_add(&(vis_acc[thread_id]), &(vis_acc[thread_id]),
                    &(vis_temp[thread_id]), &error);
        }
#pragma omp barrier
        if (error) return error;

        // Accumulate each chunk into global vis structure for this channel.
        oskar_visibilities_get_channel_amps(&vis_amp, &vis_global, c, &error);
        for (int i = 0; i < num_devices; ++i)
        {
            oskar_mem_add(&vis_amp, &vis_amp, &vis_acc[i], &error);

            // Clear thread accumulation buffer.
            oskar_mem_clear_contents(&vis_acc[i], &error);
        }
    }
    if (error) return error;

    // Add uncorrelated system noise to the visibilities.
    if (settings.interferometer.noise.enable)
    {
        int seed = settings.interferometer.noise.seed;
        error = oskar_visibilities_add_system_noise(&vis_global, &tel_cpu, seed);
        if (error) return error;
    }

    oskar_log_section(log, "Simulation completed in %.3f sec.",
            timer.elapsed() / 1e3);

    // Compute baseline u,v,w coordinates for simulation.
    oskar_Mem work_uvw(type, OSKAR_LOCATION_CPU, 3 * tel_cpu.num_stations);
    oskar_evaluate_uvw_baseline(&vis_global.uu_metres, &vis_global.vv_metres,
            &vis_global.ww_metres, tel_cpu.num_stations,
            &tel_cpu.station_x, &tel_cpu.station_y, &tel_cpu.station_z,
            tel_cpu.ra0_rad, tel_cpu.dec0_rad, settings.obs.num_time_steps,
            settings.obs.start_mjd_utc, settings.obs.dt_dump_days,
            &work_uvw, &error);
    if (error) return error;

    // Write global visibilities to disk.
    if (settings.interferometer.oskar_vis_filename)
    {
        oskar_visibilities_write(&vis_global, log,
                settings.interferometer.oskar_vis_filename, &error);
        if (error) return error;
    }

#ifndef OSKAR_NO_MS
    // Write Measurement Set.
    if (settings.interferometer.ms_filename)
    {
        error = oskar_visibilities_write_ms(&vis_global, log,
                settings.interferometer.ms_filename, true);
        if (error) return error;
    }
#endif

    // Make image(s) of the simulated visibilities if required.
    if (settings.interferometer.image_interferometer_output)
    {
        if (settings.image.oskar_image || settings.image.fits_image)
        {
            oskar_Image image;
            oskar_log_section(log, "Starting OSKAR imager...");
            timer.restart();
            error = oskar_make_image(&image, log, &vis_global, &settings.image);
            oskar_log_section(log, "Imaging completed in %.3f sec.", timer.elapsed()/1.0e3);
            if (error) return error;
            if (settings.image.oskar_image)
            {
                oskar_image_write(&image, log, settings.image.oskar_image, 0,
                        &error);
                if (error) return error;
            }
#ifndef OSKAR_NO_FITS
            if (settings.image.fits_image)
            {
                error = oskar_fits_image_write(&image, log, settings.image.fits_image);
                if (error) return error;
            }
#endif
        }
        else
        {
            oskar_log_warning(log, "No image output name specified "
                    "(skipping OSKAR imager)");
        }
    }

    // Reset all CUDA devices.
    for (int i = 0; i < num_devices; ++i)
    {
        cudaSetDevice(settings.sim.cuda_device_ids[i]);
        cudaDeviceReset();
    }

    // Free sky chunks.
    for (int i = 0; i < num_sky_chunks; ++i)
    {
        oskar_sky_model_free(&sky_chunk_cpu[i], &error);
    }
    free(sky_chunk_cpu);

    oskar_log_section(log, "Run complete.");
    return OSKAR_SUCCESS;
}
