/*
BCM2835 "GPU_FFT" release 3.0
Copyright (c) 2015, Andrew Holme.
All rights reserved.
*/

static unsigned int shader_512[] = {
    #include "shader_512.hex"
};

unsigned int gpu_fft_shader_size() {
    return sizeof(shader_512);
}

unsigned int *gpu_fft_shader_code() {
    return shader_512;
}
