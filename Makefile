target=v4l2_cam
INCLUDE=-I./include
CC=gcc
SRC=$(wildcard *.c) $(wildcard ./src/*.c)
OBJ=$(patsubst %.c,%.o,$(SRC))
LIBFLAGS=-ljpeg -lSDL2

${target}: ${OBJ}
	${CC} $^ ${INCLUDE} -o $@ $(LIBFLAGS) 

%.o: %.c
	${CC} -c ${INCLUDE} $^ -o $@ $(LIBFLAGS)

clean:
	rm ${target} ${OBJ}
