#ifndef _PARSE_ARG_H
#define _PARSE_ARG_H

struct opt_args {
	char *pixel_format;
	char *display_mode;
	char *width;
	char *height;
};


void parse_args(int fd, struct opt_args *args, int argc, char **argv)

void usage(void);

#endif
