S = hex/shader_512.hex

C = mailbox.c gpu_fft.c gpu_fft_base.c gpu_fft_twiddles.c gpu_fft_shaders.c

C1D = $(C) hello_fft.c

H1D = gpu_fft.h mailbox.h 

all:
	gcc -o pritty $(F) $(C1D) -lm -ldl

clean:
	rm -f pritty
