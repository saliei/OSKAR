/*
 * Copyright (c) 2011, The University of Oxford
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

#ifndef OSKAR_CUDAKD_PC2HT_H_
#define OSKAR_CUDAKD_PC2HT_H_

/**
 * @file oskar_cudakd_pc2ht.h
 */

#include "cuda/CudaEclipse.h"

/**
 * @brief
 * CUDA kernel to compute source position trigonometry.
 *
 * @details
 * This CUDA kernel precomputes the required trigonometry for the given
 * source positions in (azimuth, elevation).
 *
 * Each thread operates on a single source.
 *
 * The source positions are specified as (azimuth, elevation) pairs in the
 * \p spos array:
 *
 * spos.x = {azimuth}
 * spos.y = {elevation}
 *
 * The output \p trig array contains triplets of the following pre-computed
 * trigonometry:
 *
 * trig.x = {cosine azimuth}
 * trig.y = {sine azimuth}
 * trig.z = {cosine elevation}
 *
 * This kernel can be used to prepare a source distribution to generate
 * antenna signals for a 2D antenna array.
 *
 * @param[in] ns The number of source positions.
 * @param[in] spos The azimuth and elevation source coordinates in radians.
 * @param[out] trig The cosine and sine of the source azimuth and elevation
 *                   coordinates.
 */
__global__
void oskar_cudakd_pc2ht(const int ns, const double2* spos, double3* trig);

#endif // OSKAR_CUDAKD_PC2HT_H_
