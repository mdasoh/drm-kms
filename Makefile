FLAGS=`pkg-config --cflags --libs libdrm`
FLAGS+=-Wall -O0 -g
FLAGS+=-D_FILE_OFFSET_BITS=64

all:
	gcc -o modeset-drm-kms modeset-drm-kms.c $(FLAGS)
	gcc -o modeset-atomic modeset-atomic.c $(FLAGS)
