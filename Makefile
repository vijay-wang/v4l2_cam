target=v4l2_cam
INCLUDE=-I./include
CC=gcc
SRC=$(wildcard *.c) $(wildcard ./src/*.c)
OBJ=$(patsubst %.c,%.o,$(SRC))

${target}: ${OBJ}
	${CC} $^ ${INCLUDE} -o $@

%.o: %.c
	${CC} -c ${INCLUDE} $^ -o $@

clean:
	rm ${target} ${OBJ}
