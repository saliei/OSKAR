/*
 * Copyright (c) 2011-2020, The University of Oxford
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

#include "telescope/station/private_station.h"
#include "telescope/station/oskar_station.h"
#include "math/oskar_cmath.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

oskar_Station* oskar_station_create(int type, int location, int num_elements,
        int* status)
{
    int feed, dim;
    oskar_Station* model;

    /* Check the type and location. */
    if (type != OSKAR_SINGLE && type != OSKAR_DOUBLE)
    {
        *status = OSKAR_ERR_BAD_DATA_TYPE;
        return 0;
    }

    /* Allocate and initialise a station model structure. */
    model = (oskar_Station*) calloc(1, sizeof(oskar_Station));
    if (!model)
    {
        *status = OSKAR_ERR_MEMORY_ALLOC_FAILURE;
        return 0;
    }

    /* Initialise station meta data. */
    model->precision = type;
    model->mem_location = location;

    /* Create arrays. */
    for (feed = 0; feed < 1; feed++)
    {
        for (dim = 0; dim < 3; dim++)
        {
            model->element_true_enu_metres[feed][dim] =
                    oskar_mem_create(type, location, num_elements, status);
            model->element_measured_enu_metres[feed][dim] =
                    oskar_mem_create(type, location, num_elements, status);
        }
        model->element_weight[feed] =
                oskar_mem_create(type | OSKAR_COMPLEX,
                        location, num_elements, status);
        model->element_cable_length_error[feed] =
                oskar_mem_create(type, location, num_elements, status);
        model->element_gain[feed] =
                oskar_mem_create(type, location, num_elements, status);
        model->element_gain_error[feed] =
                oskar_mem_create(type, location, num_elements, status);
        model->element_phase_offset_rad[feed] =
                oskar_mem_create(type, location, num_elements, status);
        model->element_phase_error_rad[feed] =
                oskar_mem_create(type, location, num_elements, status);
    }
    for (feed = 0; feed < 2; feed++)
    {
        for (dim = 0; dim < 3; dim++)
        {
            model->element_euler_cpu[feed][dim] = oskar_mem_create(
                    OSKAR_DOUBLE, OSKAR_CPU, num_elements, status);
        }
    }
    model->element_types =
            oskar_mem_create(OSKAR_INT, location, num_elements, status);
    model->element_types_cpu =
            oskar_mem_create(OSKAR_INT, OSKAR_CPU, num_elements, status);
    model->element_mount_types_cpu =
            oskar_mem_create(OSKAR_CHAR, OSKAR_CPU, num_elements, status);
    model->permitted_beam_az_rad =
            oskar_mem_create(OSKAR_DOUBLE, OSKAR_CPU, 0, status);
    model->permitted_beam_el_rad =
            oskar_mem_create(OSKAR_DOUBLE, OSKAR_CPU, 0, status);

    /* Initialise common data. */
    model->station_type = OSKAR_STATION_TYPE_AA;
    model->normalise_final_beam = OSKAR_TRUE;
    model->beam_coord_type = OSKAR_SPHERICAL_TYPE_EQUATORIAL;
    model->noise_freq_hz = oskar_mem_create(type, OSKAR_CPU, 0, status);
    model->noise_rms_jy = oskar_mem_create(type, OSKAR_CPU, 0, status);

    /* Initialise aperture array data. */
    model->num_elements = num_elements;
    model->enable_array_pattern = OSKAR_TRUE;
    model->common_element_orientation = OSKAR_TRUE;
    model->common_pol_beams = OSKAR_TRUE;
    model->seed_time_variable_errors = 1;
    if (num_elements > 0)
    {
        oskar_mem_set_value_real(model->element_gain[0],
                1.0, 0, num_elements, status);
        oskar_mem_set_value_real(model->element_weight[0],
                1.0, 0, num_elements, status);
        memset(oskar_mem_void(model->element_mount_types_cpu), 'F',
                num_elements);
    }

    /* Return pointer to station model. */
    return model;
}

#ifdef __cplusplus
}
#endif
