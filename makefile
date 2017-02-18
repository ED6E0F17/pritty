S = shader_512.hex
C = pritty.c mailbox.c gpu_fft.c gpu_fft_base.c gpu_fft_twiddles.c gpu_fft_shaders.c blinkt.c
H = gpu_fft.h mailbox.h blinkt.h
FLAGS = -Wall -Wextra -I/opt/vc/include
LIBS = -L/opt/vc/lib -lbcm_host -lm -pthread -lwiringPi

all: $(C) $(H) $(S)
	gcc $(FLAGS) -o pritty $(F) $(C) $(LIBS)

clean:
	rm -f pritty
