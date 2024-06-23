target=v4l2_cam
INCLUDE=-I./include -I./lib/include
CROSS_COMPILE=arm-linux-gnueabihf-
CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
SRC=$(wildcard *.c) $(wildcard ./src/*.c)
OBJ=$(patsubst %.c,%.o,$(SRC))
#COMPILE_XH264=xh264
XH264_DIR=x264-snapshot-20160620-2245-stable
JOBS=32
#LIBS=-L./lib -lx264 -lpthread -lc -ldl -lm
LIBS=  -lpthread -L./lib -lx264


${target}: ${COMPILE_XH264} ${OBJ}
	${CC} ${OBJ} ${LIBS} ${INCLUDE} -o $@ $(LIBFLAGS)

%.o: %.c
	${CC} -c ${INCLUDE} $^ -o $@ $(LIBFLAGS)

${COMPILE_XH264}:
	cd ${XH264_DIR} && ./configure --host=arm-linux --prefix=$$(pwd)/.. --enable-shared --disable-asm  --cross-prefix=arm-linux-gnueabihf-  --enable-shared && make -j${JOBS} && make install

clean:
	rm ${target} ${OBJ}
