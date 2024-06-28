CONFIG_PATH=.config
-include $(CONFIG_PATH)

target=v4l2_cam
INCLUDE=-I./include -I./lib/include

ifeq ($(CONFIG_ARCH_ARM), y)
CROSS_COMPILE=arm-linux-gnueabihf-
host := arm-linux
endif

ifeq ($(CONFIG_ARCH_X64), y)
CROSS_COMPILE=/usr/bin/
host := x86_64-linux
endif

CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
SRC=$(wildcard *.c) $(wildcard ./src/*.c)
OBJ=$(patsubst %.c,%.o,$(SRC))
LIBS=  -lpthread -L./lib

ifeq ($(CONFIG_X264), y)
COMPILE_XH264=xh264
XH264_DIR=x264-snapshot-20160620-2245-stable
LIBS += -lx264
else
COMPILE_XH264=
XH264_DIR=
LIBS +=
endif

JOBS=32


${target}: ${COMPILE_XH264} ${OBJ}
	${CC} ${OBJ} ${LIBS} ${INCLUDE} -o $@ $(LIBFLAGS)

%.o: %.c
	${CC} -c ${INCLUDE} $^ -o $@ $(LIBFLAGS)

${COMPILE_XH264}:
	cd ${XH264_DIR} && ./configure --host=$(host) --prefix=$$(pwd)/.. --enable-shared --disable-asm  --cross-prefix=$(CROSS_COMPILE) && make -j${JOBS} && make install

%_defconfig:
	./tools/conf/conf --defconfig=configs/$@ Kconfig

menuconfig:
	./tools/conf/mconf Kconfig

clean:
	rm ${target} ${OBJ}

distclean:
	rm ${target} ${OBJ}; git clean -df
