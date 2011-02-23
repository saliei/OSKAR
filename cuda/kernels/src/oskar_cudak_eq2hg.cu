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

#include "cuda/kernels/oskar_cudak_eq2hg.h"

__global__
void oskar_cudak_eq2hg(int ns, const float2* radec,
        float cosLat, float sinLat, float lst, float2* azel)
{
    // Get the source ID that this thread is working on.
    const int s = blockDim.x * blockIdx.x + threadIdx.x;

    // Copy source equatorial coordinates from global memory.
    float2 src;
    if (s < ns)
    	src = radec[s];
    __syncthreads(); // Coalesce memory accesses.

    // Find azimuth and elevation.
    float cosDec, sinDec, cosHA, sinHA, t, X1, Y2;
    t = lst - src.x; // HA = LST - RA
    __sincosf(src.y, &sinDec, &cosDec);
    __sincosf(t, &sinHA, &cosHA);
    t = cosDec * cosHA;
    X1 = cosLat * sinDec - sinLat * t;
    Y2 = sinLat * sinDec + cosLat * t;
    t = -cosDec * sinHA;
    src.x = atan2f(t, X1); // Azimuth.
    t = hypotf(X1, t);
    src.y = atan2f(Y2, t); // Elevation.

    // Copy source horizontal coordinates into global memory.
    __syncthreads(); // Coalesce memory accesses.
    if (s < ns)
    	azel[s] = src;
}
