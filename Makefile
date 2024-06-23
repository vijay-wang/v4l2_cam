target=v4l2_cam
INCLUDE=-I./include
CC=arm-linux-gnueabihf-gcc
SRC=$(wildcard *.c) $(wildcard ./src/*.c)
OBJ=$(patsubst %.c,%.o,$(SRC))
COMPILE_XH264=xh264
XH264_DIR=x264-snapshot-20160620-2245-stable
JOBS=32

${target}: ${OBJ} ${COMPILE_XH264}
	${CC} ${OBJ} ${INCLUDE} -o $@ $(LIBFLAGS) 

%.o: %.c
	${CC} -c ${INCLUDE} $^ -o $@ $(LIBFLAGS)

${COMPILE_XH264}:
	cd ${XH264_DIR} && ./configure --host=arm-linux --prefix=$$(pwd)/../lib --enable-shared --disable-asm  --cross-prefix=arm-linux-gnueabihf-  --enable-shared && make -j${JOBS} && make install

clean:
	rm ${target} ${OBJ}
