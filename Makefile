target=v4l2_cam
INCLUDE=-I./include
CC=arm-linux-gnueabihf-gcc
SRC=$(wildcard *.c) $(wildcard ./src/*.c)
OBJ=$(patsubst %.c,%.o,$(SRC))

${target}: ${OBJ}
	${CC} $^ ${INCLUDE} -o $@ $(LIBFLAGS) 

%.o: %.c
	${CC} -c ${INCLUDE} $^ -o $@ $(LIBFLAGS)

clean:
	rm ${target} ${OBJ}
