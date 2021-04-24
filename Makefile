FLAGS=`pkg-config --cflags --libs libdrm`
#FLAGS+=-Wall -O0 -g
FLAGS+=-D_FILE_OFFSET_BITS=64

all: fbset.o
	gcc -o modeset-drm-kms.o -c modeset-drm-kms.c $(FLAGS)
	gcc -o modeset-drm-kms-.o -c modeset-drm-kms-.c $(FLAGS)
	gcc -o modeset-atomic modeset-atomic.c $(FLAGS)
	gcc -o modeset-drm-kms modeset-drm-kms.c $(FLAGS)
	gcc -o main.o -c main.c `pkg-config --cflags libdrm`
	gcc -o main main.o fbset.o modeset-drm-kms-.o `pkg-config --libs libdrm`

fbset.o: fbset.c fbset.h fb.h
	gcc -Wall -O2   -c -o fbset.o fbset.c

clean:
	rm -f *~ *.o modeset-atomic modeset-drm-kms
