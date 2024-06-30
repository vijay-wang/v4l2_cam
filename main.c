#include <stdio.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <stdint.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/mathematics.h>



#include "video.h"
#include "level_log.h"
#include "algorithm.h"
#include "fb_screen.h"
#include "x264.h"
#include "vencode.h"



#define BUFFER_COUNT	4
#define FILE_SIZE_LIMIT (1 * 1024 * 1024 * 1024) // 4MB
struct opt_args {
	char *pixel_format;
	char *display_mode;
	char *width;
	char *height;
};

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

static int main_run = 0;
void sighandler(int sig)
{
	main_run = 0;
}

#define FIFO_NAME "/tmp/h264_fifo"
#define OUTPUT_URL "rtmp://127.0.0.1:1935/live/mystream"
void *rtmp_stream_proces(void *arg)
{

	AVOutputFormat* ofmt = NULL;
	//输入对应一个AVFormatContext，输出对应一个AVFormatContext
	//（Input AVFormatContext and Output AVFormatContext）
	AVFormatContext* ifmt_ctx = NULL, * ofmt_ctx = NULL;
	AVPacket pkt;
	const char *in_filename = FIFO_NAME;
	const char *out_filename = OUTPUT_URL;
	int ret, i;
	int videoindex = -1;
	int frame_index = 0;
	int64_t start_time = 0;

	//注册FFmpeg所有编解码器
	av_register_all();
	//Network
	avformat_network_init();
	//输入（Input）
	if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
		printf("Could not open input file.");
		goto end;
	}
	if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
		printf("Failed to retrieve input stream information");
		goto end;
	}

	for (i = 0; i < ifmt_ctx->nb_streams; i++)
		if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoindex = i;
			break;
		}

	av_dump_format(ifmt_ctx, 0, in_filename, 0);

	//输出（Output）

	avformat_alloc_output_context2(&ofmt_ctx, NULL, "flv", out_filename); //RTMP
									      //avformat_alloc_output_context2(&ofmt_ctx, NULL, "mpegts", out_filename);//UDP

	if (!ofmt_ctx) {
		printf("Could not create output context\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}
	ofmt = ofmt_ctx->oformat;
	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		//根据输入流创建输出流（Create output AVStream according to input AVStream）
		AVStream* in_stream = ifmt_ctx->streams[i];
		AVStream* out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
		if (!out_stream) {
			printf("Failed allocating output stream\n");
			ret = AVERROR_UNKNOWN;
			goto end;
		}
		//复制AVCodecContext的设置（Copy the settings of AVCodecContext）
		ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
		if (ret < 0) {
			printf("Failed to copy context from input to output stream codec context\n");
			goto end;
		}
		out_stream->codec->codec_tag = 0;
		if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}
	//Dump Format
	av_dump_format(ofmt_ctx, 0, out_filename, 1);
	//打开输出URL（Open output URL）
	if (!(ofmt->flags & AVFMT_NOFILE)) {
		ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
		if (ret < 0) {
			printf("Could not open output URL '%s'", out_filename);
			goto end;
		}
	}
	//写文件头（Write file header）
	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0) {
		printf("Error occurred when opening output URL\n");
		goto end;
	}

	start_time = av_gettime();
	while (1) {
		AVStream* in_stream, * out_stream;
		//获取一个AVPacket（Get an AVPacket）
		ret = av_read_frame(ifmt_ctx, &pkt);
		if (ret < 0)
			break;
		//FIX：No PTS (Example: Raw H.264)
		//Simple Write PTS
		if (pkt.pts == AV_NOPTS_VALUE) {
			//Write PTS
			AVRational time_base1 = ifmt_ctx->streams[videoindex]->time_base;
			//Duration between 2 frames (us)
			int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(ifmt_ctx->streams[videoindex]->r_frame_rate);
			//Parameters
			pkt.pts = (double)(frame_index * calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
			pkt.dts = pkt.pts;
			pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);
		}
		////Important:Delay 延时
		//if (pkt.stream_index == videoindex) {
		//	AVRational time_base = ifmt_ctx->streams[videoindex]->time_base;
		//	AVRational time_base_q = {1,AV_TIME_BASE };
		//	int64_t pts_time = av_rescale_q(pkt.dts, time_base, time_base_q);
		//	int64_t now_time = av_gettime() - start_time;
		//	if (pts_time > now_time)
		//		av_usleep(pts_time - now_time);

		//}

		in_stream = ifmt_ctx->streams[pkt.stream_index];
		out_stream = ofmt_ctx->streams[pkt.stream_index];
		///*copy packet */
		////转换PTS/DTS（Convert PTS/DTS）
		////pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		////pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
		pkt.pos = -1;
		////Print to Screen
		//if (pkt.stream_index == videoindex) {
		//	printf("Send %8d video frames to output URL\n", frame_index);
		//	frame_index++;
		//}
		//ret = av_write_frame(ofmt_ctx, &pkt);
		ret = av_interleaved_write_frame(ofmt_ctx, &pkt);

		if (ret < 0) {
			printf("Error muxing packet\n");
			break;
		}

		av_free_packet(&pkt);

	}
	//写文件尾（Write file trailer）
	av_write_trailer(ofmt_ctx);
end:
	avformat_close_input(&ifmt_ctx);
	/*close output */
	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
		avio_close(ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);
	if (ret < 0 && ret != AVERROR_EOF) {
		printf("Error occurred.\n");
		return NULL;
	}
	return NULL;

}

int main(int argc, char *argv[])
{
	int fd, ret;
	unsigned char *rgb_frame;
	unsigned int width, height;
	struct mbuf bufs[BUFFER_COUNT];
	struct v4l2_format fmt;
	struct v4l2_requestbuffers reqbuffer;
	struct v4l2_buffer mbuffer;
	struct fb_info fb_info;
	struct opt_args args;

	memset(bufs, 0, sizeof(bufs));
	memset(&fmt, 0, sizeof(struct v4l2_format));
	memset(&reqbuffer, 0, sizeof(struct v4l2_requestbuffers));
	memset(&mbuffer, 0, sizeof(struct v4l2_buffer));
	memset(&fb_info, 0, sizeof(struct fb_info));
	memset(&args, 0, sizeof(struct opt_args));

	signal(SIGINT, sighandler);
	signal(SIGKILL, sighandler);
	signal(SIGSTOP, sighandler);
	signal(SIGHUP, sighandler);

	fd = camera_open(VIDEO_DEV);
	if (fd < 0) {
		LOG_ERROR("camera_open failed\n");
		return fd;
	} else
		LOG_DEBUG("camera_open success\n");

	parse_args(fd, &args, argc, argv);
	width = atoi(args.width);
	height = atoi(args.height);

	/* set format */
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = width;
	fmt.fmt.pix.height = height;
	if (!args.pixel_format) {
		LOG_ERROR("Pixel format not been set\n");
		exit(EXIT_FAILURE);
	} else if (!strcmp(args.pixel_format, "yuyv"))
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	else if (!strcmp(args.pixel_format, "rgb565le"))
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
	else {
		LOG_ERROR("Unkown pixel format\n");
		exit(EXIT_FAILURE);
	}
	ret = camera_set_format(fd, &fmt);
	if (ret < 0)
		LOG_ERROR("camera_set_format failed\n");
	 else
		LOG_DEBUG("camera_set_format success\n");

	/* request buffers */
	reqbuffer.count = BUFFER_COUNT;
	reqbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuffer.memory = V4L2_MEMORY_MMAP;
	ret = camera_request_buffers(fd, &reqbuffer);
	if (ret < 0)
		LOG_ERROR("camera_request_buffers failed\n");
	 else
		LOG_DEBUG("camera_request_buffers success\n");

	/* query buffers, map buffers, enqueue buffers */
	memset(&mbuffer, 0, sizeof(struct v4l2_buffer));
	int i;
	for (i = 0; i < BUFFER_COUNT; ++i) {
		mbuffer.index = i;
		mbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		ret = camera_query_buffer(fd, &mbuffer);
		if (ret < 0)
			LOG_ERROR("camera_query_buffers failed\n");
		ret = camera_map_buffer(fd, &mbuffer, &bufs[i]);
		if (ret < 0)
			LOG_ERROR("camera_map_buffers failed\n");
		ret = camera_qbuffer(fd, &mbuffer);
		if (ret < 0)
			LOG_ERROR("camera_qbuffer failed\n");
	}

	/* stream on */
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = camera_streamon(fd, &type);
	if (ret < 0)
		LOG_ERROR("camera_streamon failed\n");
	 else
		LOG_DEBUG("camera_streamon success\n");

	rgb_frame = (unsigned char *)camera_alloc_rgb(width, height);

	if (!args.display_mode) {
		LOG_ERROR("Display mode not been set\n");
		exit(EXIT_FAILURE);
	} else if (!strcmp(args.display_mode, "fb")) {
		fb_set_info(&fb_info, width, height, FB_PATH);
		ret = fb_init(&fb_info);
		if (ret < 0)
			LOG_ERROR("fb_init failed\n");
	} else {
		LOG_ERROR("Unkown display_mode\n");
		exit(EXIT_FAILURE);
	}

	// Initialize x264 encoder
	x264_picture_t pic_in, pic_out;
	x264_param_t param;
	x264_t *encoder;
	x264_nal_t *nals;
	int num_nals;

	encoder = init_x264_encoder(&param, &pic_in, width, height);

	main_run = 1;

	int file_count = 0;
	int file_size = 0;
	int file = NULL;
	char filename[64];

	ret = mkfifo(FIFO_NAME, 0777);
	if (ret < 0)
		LOG_ERROR("mkfifo failed\n");

	pthread_t tid;
	pthread_create(&tid, NULL, rtmp_stream_proces, NULL);


	file = open(FIFO_NAME, O_WRONLY);
	if (!file) {
		LOG_ERROR("Error opening output file");
		return -1;
	}

	while (main_run) {
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		struct timeval tv = {0};
		tv.tv_sec = 5;
		int r = select(fd + 1, &fds, NULL, NULL, &tv);
		if (-1 == r) {
			LOG_WARNING("Waiting for Frame time out 5s\n");
			//break;
			continue;
		}

		if (camera_dqbuffer(fd, &mbuffer) == -1) {
			LOG_WARNING("Retrieving Frame failed\n");
			continue;
			//return 1;
		}

		yuyv_to_yuv420p((unsigned char*)bufs[mbuffer.index].pbuf, &pic_in, width, height);
		if (x264_encoder_encode(encoder, &nals, &num_nals, &pic_in, &pic_out) < 0) {
			LOG_ERROR("Error encoding frame\n");
			break;
		}



		int i;
		for (i = 0; i < num_nals; ++i) {
			ret = write(file, nals[i].p_payload, nals[i].i_payload);
			//printf("write frame bytes: %d\n", ret);
		}


		if (camera_qbuffer(fd, &mbuffer) == -1) {
			LOG_WARNING("Queue Buffer failed\n");
			continue;
			//break;
		}
	}
	close(file);

	x264_picture_clean(&pic_in);
	x264_encoder_close(encoder);

	if (!strcmp(args.display_mode, "fb")) {
		ret = fb_deinit(&fb_info);
		if (ret < 0)
			LOG_ERROR("fb_deinit failed\n");
	}
	camera_free_rgb(rgb_frame);
		
	/* stream off */
	ret = camera_streamoff(fd, &type);
	if (ret < 0)
		LOG_ERROR("camera_streamoff failed\n");
	 else
		LOG_DEBUG("camera_streamoff success\n");

	/* unmap buffers */
	for (i = 0; i < BUFFER_COUNT; ++i) {
		ret = camera_munmap_buffer(&bufs[i]);
		if (ret < 0)
			LOG_ERROR("camera_munmap_buffer failed\n");
	}
	camera_close(fd);
	return 0;
}
