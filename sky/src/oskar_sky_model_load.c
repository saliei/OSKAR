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

#include "sky/oskar_sky_model_append.h"
#include "sky/oskar_sky_model_free.h"
#include "sky/oskar_sky_model_init.h"
#include "sky/oskar_sky_model_load.h"
#include "sky/oskar_sky_model_resize.h"
#include "sky/oskar_sky_model_set_source.h"
#include "sky/oskar_sky_model_type.h"
#include "utility/oskar_getline.h"
#include "utility/oskar_string_to_array.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

static const double deg2rad = 1.74532925199432957692369e-2;
static const double arcsec2rad = 4.84813681109535993589914e-6;

void oskar_sky_model_load(oskar_SkyModel* sky, const char* filename,
        int* status)
{
    int type, n = 0;
    FILE* file;
    char* line = NULL;
    size_t bufsize = 0;
    oskar_SkyModel temp_sky;

    /* Check all inputs. */
    if (!sky || !filename || !status)
    {
        oskar_set_invalid_argument(status);
        return;
    }

    /* Check if safe to proceed. */
    if (*status) return;

    /* Get the data type. */
    type = oskar_sky_model_type(sky);
    if (type != OSKAR_SINGLE && type != OSKAR_DOUBLE)
    {
        *status = OSKAR_ERR_BAD_DATA_TYPE;
        return;
    }

    /* Open the file. */
    file = fopen(filename, "r");
    if (file == NULL)
    {
        *status = OSKAR_ERR_FILE_IO;
        return;
    }

    /* Initialise the temporary sky model. */
    oskar_sky_model_init(&temp_sky, type, OSKAR_LOCATION_CPU, 0, status);

    if (type == OSKAR_DOUBLE)
    {
        while (oskar_getline(&line, &bufsize, file) != OSKAR_ERR_EOF)
        {
            /* Set defaults. */
            /*  (RA, Dec, I, Q, U, V, freq0, spix, FWHM maj, FWHM min, PA) */
            int num_param = 11;
            int num_required = 3;
            double par[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

            /* Ignore comment lines (lines starting with '#') */
            if (line[0] == '#') continue;

            /* Load source parameters (require at least RA, Dec, Stokes I). */
            if (oskar_string_to_array_d(line, num_param, par) < num_required)
                continue;

            /* Ensure enough space in arrays. */
            if (n % 100 == 0)
            {
                oskar_sky_model_resize(&temp_sky, n + 100, status);
                if (*status)
                {
                    oskar_sky_model_free(&temp_sky, status);
                    fclose(file);
                    return;
                }
            }
            oskar_sky_model_set_source(&temp_sky, n,
                    par[0] * deg2rad, par[1] * deg2rad,
                    par[2], par[3], par[4], par[5], par[6], par[7],
                    par[8] * arcsec2rad, par[9] * arcsec2rad, par[10] * deg2rad,
                    status);
            ++n;
        }
    }
    else if (type == OSKAR_SINGLE)
    {
        while (oskar_getline(&line, &bufsize, file) != OSKAR_ERR_EOF)
        {
            /* Set defaults. */
            /*  (RA, Dec, I, Q, U, V, freq0, spix, FWHM maj, FWHM min, PA) */
            int num_param = 11;
            int num_required = 3;
            float par[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

            /* Ignore comment lines (lines starting with '#') */
            if (line[0] == '#') continue;

            /* Load source parameters (require at least RA, Dec, Stokes I). */
            if (oskar_string_to_array_f(line, num_param, par) < num_required)
                continue;

            /* Ensure enough space in arrays. */
            if (n % 100 == 0)
            {
                oskar_sky_model_resize(&temp_sky, n + 100, status);
                if (*status)
                {
                    oskar_sky_model_free(&temp_sky, status);
                    fclose(file);
                    return;
                }
            }
            oskar_sky_model_set_source(&temp_sky, n,
                    par[0] * deg2rad, par[1] * deg2rad,
                    par[2], par[3], par[4], par[5], par[6], par[7],
                    par[8] * arcsec2rad, par[9] * arcsec2rad, par[10] * deg2rad,
                    status);
            ++n;
        }
    }

    /* Record the number of elements loaded. */
    temp_sky.num_sources = n;
    oskar_sky_model_append(sky, &temp_sky, status);

    /* Free the temporary sky model. */
    oskar_sky_model_free(&temp_sky, status);

    /* Free the line buffer and close the file. */
    if (line) free(line);
    fclose(file);
}

#ifdef __cplusplus
}
#endif
