S = shader_512.hex
C = pritty.c mailbox.c gpu_fft.c gpu_fft_base.c gpu_fft_twiddles.c gpu_fft_shaders.c
H = gpu_fft.h mailbox.h 
FLAGS = -Wall -Wextra
LIBS = -ldl -lm

all: $(C) $(H) $(S)
	gcc $(FLAGS) -o pritty $(F) $(C) $(LIBS)

clean:
	rm -f pritty
