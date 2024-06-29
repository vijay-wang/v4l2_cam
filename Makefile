target=v4l2_cam
INCLUDE=-I./include -I./lib/include 
CROSS_COMPILE=arm-linux-gnueabihf-
CC=$(CROSS_COMPILE)gcc
#CC=$(CROSS_COMPILE)g++
LD=$(CROSS_COMPILE)ld
SRC=$(wildcard *.c) $(wildcard ./src/*.c)
OBJ=$(patsubst %.c,%.o,$(SRC))
COMPILE_XH264=xh264
XH264_DIR=x264-snapshot-20160620-2245-stable
JOBS=32
LIBS=  -lpthread -L./lib -lx264
#CPPFLAGS=-std=c++11


${target}: ${COMPILE_XH264} ${OBJ}
	${CC} ${OBJ} ${LIBS} ${INCLUDE} -o $@ $(LIBFLAGS) $(CPPFLAGS)

%.o: %.c
	${CC} -c ${INCLUDE} $^ -o $@ $(LIBFLAGS) $(CPPFLAGS)

${COMPILE_XH264}:
	cd ${XH264_DIR} && ./configure --host=arm-linux --prefix=$$(pwd)/.. --enable-shared --disable-asm  --cross-prefix=arm-linux-gnueabihf-  && make -j${JOBS} && make install

clean:
	rm ${target} ${OBJ}
