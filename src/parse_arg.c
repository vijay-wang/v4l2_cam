#include "parse_arg.h"

void usage(void)
{
	printf(	"v4l2_cam usage:\n"
		"	-f pixel format, yuyv, rgb565le\n"
		"	-W set pixel width\n"
		"	-H set pixel height\n"
		"	-d set display way\n"
		"	-l list the resolutions supported by the camera\n"
		"	-q query the basic infomation about the driver and camera\n"
		"	-h help\n");
}

void parse_args(int fd, struct opt_args *args, int argc, char **argv)
{
	int ch;

	if (argc < 2) {
		usage();
		exit(EXIT_FAILURE);
	}

	while ((ch = getopt(argc, argv, "d:f:W:H:lqh")) != -1)  {
		switch (ch) {
		case 'd':
			args->display_mode = optarg;
			break;
		case 'f':
			args->pixel_format = optarg;
			break;
		case 'W':
			args->width = optarg;
			break;
		case 'H':
			args->height = optarg;
			break;
		case 'q':
			camera_query_capability(fd);
			break;
		case 'l':
			camera_list_fmt(fd);
			break;

		case 'h':
			usage();
			camera_close(fd);
			exit(EXIT_SUCCESS);
		default:
			usage();
			camera_close(fd);
			exit(EXIT_FAILURE);
		}
	}

	int i;
	for (i = optind; i < argc; i++) {
		LOG_ERROR("Error: Invalid argument '%s'\n", argv[i]);
		usage();
		exit(EXIT_FAILURE);
	}
}
