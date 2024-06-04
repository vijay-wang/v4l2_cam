target=v4l2_cam
all:
	gcc main.c -o ${target}

clean:
	rm ${target}
