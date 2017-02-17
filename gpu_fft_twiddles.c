/*
BCM2835 "GPU_FFT" release 3.0
Copyright (c) 2015, Andrew Holme.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <math.h>

#include "gpu_fft.h"

#define ALPHA(dx) (2*powf(sinf((dx)/2),2))
#define  BETA(dx) (sinf(dx))

static float k[16] = {0,8,4,4,2,2,2,2,1,1,1,1,1,1,1,1};
static float m[16] = {0,0,0,1,0,1,2,3,0,1,2,3,4,5,6,7};

/****************************************************************************/

static float *twiddles_base_16(float two_pi, float *out, float theta) {
    int i;
    for (i=0; i<16; i++) {
        *out++ = cosf(two_pi/16*k[i]*m[i] + theta*k[i]);
        *out++ = sinf(two_pi/16*k[i]*m[i] + theta*k[i]);
    }
    return out;
}

static float *twiddles_base_32(float two_pi, float *out, float theta) {
    int i;
    for (i=0; i<16; i++) {
        *out++ = cosf(two_pi/32*i + theta);
        *out++ = sinf(two_pi/32*i + theta);
    }
    return twiddles_base_16(two_pi, out, 2*theta);
}

/****************************************************************************/

static float *twiddles_step_16(float *out, float theta) {
    int i;
    for (i=0; i<16; i++) {
        *out++ = ALPHA(theta*k[i]);
        *out++ =  BETA(theta*k[i]);
    }
    return out;
}

static float *twiddles_step_32(float *out, float theta) {
    int i;
    for (i=0; i<16; i++) {
        *out++ = ALPHA(theta);
        *out++ =  BETA(theta);
    }
    return twiddles_step_16(out, 2*theta);
}

/****************************************************************************/

static void twiddles_256(float two_pi, float *out) {
    float N=256;
    int q;

    out = twiddles_base_16(two_pi, out, 0);
    out = twiddles_step_16(out, two_pi/N * GPU_FFT_QPUS);

    for (q=0; q<GPU_FFT_QPUS; q++)
        out = twiddles_base_16(two_pi, out, two_pi/N*q);
}

static void twiddles_512(float two_pi, float *out) {
    float N=512;
    int q;

    out = twiddles_base_32(two_pi, out, 0);
    out = twiddles_step_16(out, two_pi/N * GPU_FFT_QPUS);

    for (q=0; q<GPU_FFT_QPUS; q++)
        out = twiddles_base_16(two_pi, out, two_pi/N*q);
}

static void twiddles_1k(float two_pi, float *out) {
    float N=1024;
    int q;

    out = twiddles_base_32(two_pi, out, 0);
    out = twiddles_step_32(out, two_pi/N * GPU_FFT_QPUS);

    for (q=0; q<GPU_FFT_QPUS; q++)
        out = twiddles_base_32(two_pi, out, two_pi/N*q);
}

/****************************************************************************/

static struct {
    int passes, shared, unique;
    void (*twiddles)(float, float *);
}
shaders[] = {
    {2, 2, 1, twiddles_256},
    {2, 3, 1, twiddles_512},
    {2, 4, 2, twiddles_1k}
};

int gpu_fft_twiddle_size(int log2_N, int *shared, int *unique, int *passes) {
    if (log2_N<8 || log2_N>10) return -1;
    *shared = shaders[log2_N-8].shared;
    *unique = shaders[log2_N-8].unique;
    *passes = shaders[log2_N-8].passes;
    return 0;
}

void gpu_fft_twiddle_data(int log2_N, int direction, float *out) {
    shaders[log2_N-8].twiddles((direction==GPU_FFT_FWD?-2:2)*GPU_FFT_PI, out);
}
