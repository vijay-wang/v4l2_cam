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
#include <stdatomic.h>
#include <semaphore.h> 
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

#define BUFFER_COUNT	6
#define  QDEEPTH	BUFFER_COUNT

struct frm_queue {
	//struct v4l2_buffer *blks[QDEEPTH];
	struct v4l2_buffer mbuffer[QDEEPTH];
	struct mbuf bufs[QDEEPTH];
	unsigned char front;
	unsigned char rear;
	_Atomic unsigned char count;	//the queue member nums
	_Atomic unsigned char keep_num; //the queue member num keeped bey user
	sem_t sem;
};

static struct frm_queue data_frames;
static int qdeepth;

static int enqueue(struct frm_queue *q)
{
	if (atomic_load(&q->count) < qdeepth) {
		q->rear = (++q->rear) % qdeepth;
		atomic_fetch_add(&q->count, 1);
		sem_post(&q->sem);
	} else {
		LOG_ERROR("libunicam queue full\n");
		return -1;
	}
	return 0;
}

static int dequeue(struct frm_queue *q)
{
	sem_wait(&q->sem);
	if (atomic_load(&q->count) == 0) {
		LOG_ERROR("libunicam queue empty\n");
		return -1;
	} else {
		q->front = (++q->front) % qdeepth;
		atomic_fetch_sub(&q->keep_num, 1);
		atomic_fetch_sub(&q->count, 1);
	}
	return 0;
}
static int queue_init(struct frm_queue *q)
{
	q->front = 0;
	q->rear  = 0;

	if (qdeepth == 0)
		qdeepth = QDEEPTH;

	atomic_init(&q->count, 0);
	atomic_init(&q->keep_num, 0);
	sem_init(&q->sem, 0, 0);
	return 0;
}


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

static double r2d(AVRational r)
{
	    return r.num == 0 || r.den == 0 ? 0. : (double)r.num / (double)r.den;
}





int avError(int errNum) {
	char buf[1024];
	//获取错误信息
	av_strerror(errNum, buf, sizeof(buf));
	LOG_ERROR("failed: %s\n", buf);
	return -1;
}



#define FIFO_NAME "/tmp/h264_fifo"
#define OUTPUT_URL "rtmp://127.0.0.1:1935/live/mystream"
void *rtmp_stream_proces(void *arg)
{
	int videoindex = -1;
	//所有代码执行之前要调用av_register_all和avformat_network_init
	//    //初始化所有的封装和解封装 flv mp4 mp3 mov。不包含编码和解码
	av_register_all();

	//初始化网络库
	avformat_network_init();

	//使用的相对路径，执行文件在bin目录下。test.mp4放到bin目录下即可
	const char *inUrl = "/tmp/h264_fifo";
	//输出的地址
	const char *outUrl = "rtmp://127.0.0.1:1935/live/mystream";

	//////////////////////////////////////////////////////////////////
	//                   输入流处理部分
	/////////////////////////////////////////////////////////////////
	//打开文件，解封装 avformat_open_input
	//AVFormatContext **ps  输入封装的上下文。包含所有的格式内容和所有的IO。如果是文件就是文件IO，网络就对应网络IO
	//const char *url  路径
	//AVInputFormt * fmt 封装器
	//AVDictionary ** options 参数设置
	AVFormatContext *ictx = NULL;

	AVOutputFormat *ofmt = NULL;

	//打开文件，解封文件头
	int ret = avformat_open_input(&ictx, inUrl, 0, NULL);
	if (ret < 0) {
		LOG_INFO("avformat_open_input failed!\n");
		return NULL;
	}
	LOG_INFO("avformat_open_input success!\n");
	//获取音频视频的信息 .h264 flv 没有头信息
	ret = avformat_find_stream_info(ictx, 0);
	if (ret != 0) {
		LOG_INFO(" avformat_find_stream_info failed!\n");
		return NULL;
	}
	//打印视频视频信息
	//0打印所有  inUrl 打印时候显示，
	av_dump_format(ictx, 0, inUrl, 0);

	//////////////////////////////////////////////////////////////////
	//                   输出流处理部分
	/////////////////////////////////////////////////////////////////
	AVFormatContext * octx = NULL;
	//如果是输入文件 flv可以不传，可以从文件中判断。如果是流则必须传
	//创建输出上下文
	ret = avformat_alloc_output_context2(&octx, NULL, "flv", outUrl);
	if (ret < 0) {
		LOG_INFO(" avformat_alloc_output_context2 failed!\n");
		return NULL;
	}
	LOG_INFO("avformat_alloc_output_context2 success!\n");

	ofmt = octx->oformat;
	LOG_INFO("nb_streams\n");
	int i;

	for (i = 0; i < ictx->nb_streams; i++) {

		//获取输入视频流
		AVStream *in_stream = ictx->streams[i];
		//为输出上下文添加音视频流（初始化一个音视频流容器）
		AVStream *out_stream = avformat_new_stream(octx, in_stream->codec->codec);
		if (!out_stream) {
			printf("未能成功添加音视频流\n");
			ret = AVERROR_UNKNOWN;
		}

		//将输入编解码器上下文信息 copy 给输出编解码器上下文
		//ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
		ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
		//ret = avcodec_parameters_from_context(out_stream->codecpar, in_stream->codec);
		//ret = avcodec_parameters_to_context(out_stream->codec, in_stream->codecpar);
		if (ret < 0) {
			printf("copy 编解码器上下文失败\n");
		}
		out_stream->codecpar->codec_tag = 0;

		out_stream->codec->codec_tag = 0;
		if (octx->oformat->flags & AVFMT_GLOBALHEADER) {
			out_stream->codec->flags = out_stream->codec->flags | CODEC_FLAG_GLOBAL_HEADER;
		}
	}

	//输入流数据的数量循环
	for (i = 0; i < ictx->nb_streams; i++) {
		if (ictx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoindex = i;
			break;
		}
	}

	av_dump_format(octx, 0, outUrl, 1);

	//////////////////////////////////////////////////////////////////
	//                   准备推流
	/////////////////////////////////////////////////////////////////

	//打开IO
	ret = avio_open(&octx->pb, outUrl, AVIO_FLAG_WRITE);
	if (ret < 0) {
		avError(ret);
	}

	//写入头部信息
	ret = avformat_write_header(octx, 0);
	if (ret < 0) {
		avError(ret);
	}
	LOG_INFO("avformat_write_header Success!\n");
	//推流每一帧数据
	//int64_t pts  [pts*(num/den)  第几秒显示]
	//int64_t dts  解码时间 [P帧(相对于上一帧的变化) I帧(关键帧，完整的数据) B帧(上一帧和下一帧的变化)]  有了B帧压缩率更高。
	//uint8_t *data    
	//int size
	//int stream_index
	//int flag
	AVPacket pkt;
	//获取当前的时间戳  微妙
	long long start_time = av_gettime();
	long long frame_index = 0;
	while (1) {
		//输入输出视频流
		AVStream *in_stream, *out_stream;
		//获取解码前数据
		ret = av_read_frame(ictx, &pkt);
		if (ret < 0) {
			break;
		}

		/*
		   PTS（Presentation Time Stamp）显示播放时间
		   DTS（Decoding Time Stamp）解码时间
		   */
		//没有显示时间（比如未解码的 H.264 ）
		if (pkt.pts == AV_NOPTS_VALUE) {
			//AVRational time_base：时基。通过该值可以把PTS，DTS转化为真正的时间。
			AVRational time_base1 = ictx->streams[videoindex]->time_base;

			//计算两帧之间的时间
			/*
			   r_frame_rate 基流帧速率  （不是太懂）
			   av_q2d 转化为double类型
			   */
			int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(ictx->streams[videoindex]->r_frame_rate);

			//配置参数
			pkt.pts = (double)(frame_index*calc_duration) / (double)(av_q2d(time_base1)*AV_TIME_BASE);
			pkt.dts = pkt.pts;
			pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1)*AV_TIME_BASE);
		}

		//延时
		if (pkt.stream_index == videoindex) {
			AVRational time_base = ictx->streams[videoindex]->time_base;
			AVRational time_base_q = {1,AV_TIME_BASE };
			//计算视频播放时间
			int64_t pts_time = av_rescale_q(pkt.dts, time_base, time_base_q);
			//计算实际视频的播放时间
			int64_t now_time = av_gettime() - start_time;

			AVRational avr = ictx->streams[videoindex]->time_base;
			//LOG_INFO("avr.num: %d, avr.den: %d, pkt.dts: %d, pkt.pts: %d, pts_time: %d\n", avr.num, avr.den, pkt.dts, pkt.pts, pts_time);
			if (pts_time > now_time) {
				//睡眠一段时间（目的是让当前视频记录的播放时间与实际时间同步）
				av_usleep((unsigned int)(pts_time - now_time));
			}
		}

		in_stream = ictx->streams[pkt.stream_index];
		out_stream = octx->streams[pkt.stream_index];

		//计算延时后，重新指定时间戳
		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base,(enum AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (enum AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.duration = (int)av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
		//字节流的位置，-1 表示不知道字节流位置
		pkt.pos = -1;

		if (pkt.stream_index == videoindex) {
			//printf("Send %8d video frames to output URL\n", frame_index);
			frame_index++;
		}

		//向输出上下文发送（向地址推送）
		ret = av_interleaved_write_frame(octx, &pkt);

		if (ret < 0) {
			printf("发送数据包出错\n");
			break;
		}

		//释放
		av_free_packet(&pkt);
	}
	return 0;
}

// Initialize x264 encoder
x264_picture_t pic_in, pic_out;
unsigned int width, height;
x264_param_t param;
x264_t *encoder;
x264_nal_t *nals;
int num_nals;
int file = NULL;

//camera file descriptor
int fd;
void *encode_process(void *arg)
{
	
	int ret;
	while(1) {

		yuyv_to_yuv420p((unsigned char*)data_frames.bufs[data_frames.front].pbuf, &pic_in, width, height);
		if (x264_encoder_encode(encoder, &nals, &num_nals, &pic_in, &pic_out) < 0) {
			LOG_ERROR("Error encoding frame\n");
			break;
		}



		int i;
		for (i = 0; i < num_nals; ++i) {
			ret = write(file, nals[i].p_payload, nals[i].i_payload);
			//printf("write frame bytes: %d\n", ret);
		}


		if (camera_qbuffer(fd, &data_frames.mbuffer[data_frames.front]) == -1) {
			LOG_WARNING("Queue Buffer failed\n");
			continue;
			//break;
		}
		dequeue(&data_frames);
	}
	return NULL;
}

int main(int argc, char *argv[])
{



	//int fd;
	int ret;
	unsigned char *rgb_frame;
	memset(&data_frames, 0, sizeof(struct frm_queue));
	//unsigned int width, height;
	//struct mbuf bufs[BUFFER_COUNT];
	struct v4l2_format fmt;
	struct v4l2_requestbuffers reqbuffer;
	//struct v4l2_buffer mbuffer;
	struct fb_info fb_info;
	struct opt_args args;

	//init queue
	ret = queue_init(&data_frames);
	if (ret)
		return ret;

	//memset(bufs, 0, sizeof(bufs));
	memset(&fmt, 0, sizeof(struct v4l2_format));
	memset(&reqbuffer, 0, sizeof(struct v4l2_requestbuffers));
	//memset(&mbuffer, 0, sizeof(struct v4l2_buffer));
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
	//memset(&mbuffer, 0, sizeof(struct v4l2_buffer));
	int i;
	for (i = 0; i < BUFFER_COUNT; ++i) {
		data_frames.mbuffer[i].index = i;
		data_frames.mbuffer[i].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		ret = camera_query_buffer(fd, &data_frames.mbuffer[i]);
		if (ret < 0)
			LOG_ERROR("camera_query_buffers failed\n");
		ret = camera_map_buffer(fd, &data_frames.mbuffer[i], &data_frames.bufs[i]);
		if (ret < 0)
			LOG_ERROR("camera_map_buffers failed\n");
		ret = camera_qbuffer(fd, &data_frames.mbuffer[i]);
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
	//x264_picture_t pic_in, pic_out;
	//x264_param_t param;
	//x264_t *encoder;
	//x264_nal_t *nals;
	//int num_nals;

	encoder = init_x264_encoder(&param, &pic_in, width, height);

	main_run = 1;

	int file_count = 0;
	int file_size = 0;
	//int file = NULL;
	char filename[64];

	ret = mkfifo(FIFO_NAME, 0777);
	if (ret < 0)
		LOG_ERROR("mkfifo failed\n");

	pthread_t rtmp_tid, encode_tid;
	//pthread_t tid;
	pthread_create(&rtmp_tid, NULL, rtmp_stream_proces, NULL);
	pthread_create(&encode_tid, NULL, encode_process, NULL);


	file = open(FIFO_NAME, O_WRONLY);
	if (!file) {
		LOG_ERROR("Error opening output file");
		return -1;
	}

    //struct v4l2_streamparm stream_params = {};
    //stream_params.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    //if (ioctl(fd, VIDIOC_G_PARM, &stream_params) == -1) {
    //    perror("Getting frame rate");
    //    return 1;
    //}






    //stream_params.parm.capture.timeperframe.numerator = 1;
    //stream_params.parm.capture.timeperframe.denominator = 30;

    //if (ioctl(fd, VIDIOC_S_PARM, &stream_params) == -1) {
    //    perror("Setting frame rate");
    //    return 1;
    //}





	while (main_run) {
		struct timespec start, end;
		clock_gettime(CLOCK_MONOTONIC, &start);

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

		if (camera_dqbuffer(fd, &data_frames.mbuffer[data_frames.rear]) == -1) {
			LOG_WARNING("Retrieving Frame failed\n");
			continue;
			//return 1;
		}
		enqueue(&data_frames);

		clock_gettime(CLOCK_MONOTONIC, &end);

		// 计算运行时间
		double time_taken = (end.tv_sec - start.tv_sec) + 
			(end.tv_nsec - start.tv_nsec) / 1000000.0;
		printf("runtime: %.9f ms\n", time_taken);



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
		ret = camera_munmap_buffer(&data_frames.bufs[i]);
		if (ret < 0)
			LOG_ERROR("camera_munmap_buffer failed\n");
	}
	camera_close(fd);
	return 0;
}
