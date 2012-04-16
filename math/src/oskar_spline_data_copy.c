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

#include "math/oskar_spline_data_copy.h"
#include "utility/oskar_mem_copy.h"

#ifdef __cplusplus
extern "C" {
#endif

int oskar_spline_data_copy(oskar_SplineData* dst, const oskar_SplineData* src)
{
    int err = 0;
    dst->num_knots_x = src->num_knots_x;
    dst->num_knots_y = src->num_knots_y;
    err = oskar_mem_copy(&dst->knots_x, &src->knots_x);
    if (err) return err;
    err = oskar_mem_copy(&dst->knots_y, &src->knots_y);
    if (err) return err;
    err = oskar_mem_copy(&dst->coeff, &src->coeff);
    if (err) return err;

    /* FIXME deprecated! */
    dst->num_knots_x_re = src->num_knots_x_re;
    dst->num_knots_y_re = src->num_knots_y_re;
    dst->num_knots_x_im = src->num_knots_x_im;
    dst->num_knots_y_im = src->num_knots_y_im;
    err = oskar_mem_copy(&dst->knots_x_re, &src->knots_x_re);
    if (err) return err;
    err = oskar_mem_copy(&dst->knots_y_re, &src->knots_y_re);
    if (err) return err;
    err = oskar_mem_copy(&dst->coeff_re, &src->coeff_re);
    if (err) return err;
    err = oskar_mem_copy(&dst->knots_x_im, &src->knots_x_im);
    if (err) return err;
    err = oskar_mem_copy(&dst->knots_y_im, &src->knots_y_im);
    if (err) return err;
    err = oskar_mem_copy(&dst->coeff_im, &src->coeff_im);
    if (err) return err;

    return 0;
}

#ifdef __cplusplus
}
#endif
