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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "mailbox.h"
#include "gpu_fft.h"
#include "blinkt.h"

/*
 * 48khz IQ s16le
 * 512 element FFT for 94Hz frequency resolution
 * 300 baud = 160 samples per bit
*/
#define I2Q2(i) (base[i].re * base[i].re +  base[i].im * base[i].im)

char Usage[] =
	"Usage: hello_fft.bin log2_N [jobs [loops]]\n"
	"log2_N = log2(FFT_length),       log2_N = 8...22\n"
	"jobs   = transforms per batch,   jobs>0,        default 1\n"
	"loops  = number of test repeats, loops>0,       default 1\n";

unsigned Microseconds( void ) {
	struct timespec ts;
	clock_gettime( CLOCK_REALTIME, &ts );
	return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

int main( int argc, char *argv[] ) {
	unsigned int i, j, jobs, N, log2_N, t[2];
	struct GPU_FFT_COMPLEX *base;
	struct GPU_FFT *fft;

	for (int c = 1; c < argc; c++)
		printf("Commandline arg:%s\n", argv[c]);

	log2_N = 9;
	jobs   = 4;  // oversample 4x per bit period
	N = 1 << log2_N; // FFT length 512

	int mb = mbox_open(); // Will exit() on fail - run this first.
	if (blinkt_setup() < 0) {
		 printf( "Unable to setup blinkenlights: Install wiringPi.\n" );
		 return -1;
	}

	int ret = gpu_fft_prepare( mb, GPU_FFT_FWD, jobs, &fft ); // call once
	switch ( ret ) {
	case -1: printf( "Unable to enable V3D. Please check your firmware is up to date.\n" ); return -1;
	case -2: printf( "log2_N=%d not supported.  Try between 8 and 10.\n", log2_N );         return -1;
	case -3: printf( "Out of memory.  Try a smaller batch or increase GPU memory.\n" );     return -1;
	case -4: printf( "Unable to map Videocore peripherals into ARM memory space.\n" );      return -1;
	case -5: printf( "Can't open libbcm_host.\n" );                                         return -1;
	}

	int freq = 16;
	float raisedcos[256];
	for ( j = 0; j < 256; j++ )
		raisedcos[j] = 1.0f + cosf( GPU_FFT_PI * ((float)j / 128.0f - 1.0f) );
	float I[N], Q[N];


	for ( j = 0; j < N; j++ ) {
		I[j] = sinf( 2.0 * GPU_FFT_PI * freq * j / N );
		Q[j] = 0;
	}

	usleep( 1 );   // Yield to OS
	t[0] = Microseconds();

	for ( j = 0; j < jobs; j++ ) {
		base = fft->in + j * fft->step;   // input buffer
		for ( i = 0; i < 256; i++ ) {
			base[i].re = raisedcos[i] * I[i + 40 * j];
			base[i + 256].re = 0;
			base[i].im = raisedcos[i] * Q[i + 40 * j];
			base[i + 256].im = 0;
		}
	}

	gpu_fft_execute( fft );   // call one or many times

	unsigned int fftout[N * jobs];
	unsigned int count = 0;
	for (j=0; j<jobs; j++) {
		base = fft->out + j*fft->step;
		for ( i = 0; i < 96; i+=3 ) {
			float i2q2 = I2Q2(i) + I2Q2(i + 1) + I2Q2(i + 2);
			fftout[count++] = (unsigned int)sqrtf(i2q2);
		}
	} // output buffer

	t[1] = Microseconds();
	for ( i = 0; i < 7; i++ ) {
		uint8_t r = 255 & fftout[i + 3];
		uint8_t g = 8;
		blinkt_colour(i, r, g, 0);
	}
	show_blinkt();
	printf( "\nFFT usecs = %d\n", ( t[1] - t[0] ) / jobs );

	//blinkt_clear();
	gpu_fft_release( fft ); // Videocore memory lost if not freed !
	return 0;
}
