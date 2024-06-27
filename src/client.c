#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "video.h"
#include "level_log.h"
#include "client.h"

int client_init(char *server_ip, unsigned short server_port, char *buffer, unsigned int pixel_format, unsigned short width, unsigned short height)
{
	int sock;
	struct sockaddr_in server_addr;

	// 创建套接字
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		LOG_ERROR("socket failed\n");
		exit(EXIT_FAILURE);
	}

	// 设置服务器地址结构体
	memset(&server_addr, 0 ,sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server_port);
	inet_aton(server_ip, &server_addr.sin_addr);	

	// 连接服务器
	while (1) {
		if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
			LOG_ERROR("connect failed\n");
		else
			break;
		sleep(2);
		
	}
	//unsigned short *p = (unsigned short *)buffer;
	//*(p) = 0xaa55;
	//*(p + 1) = pixel_format;
	//*(p + 3) = width;
	//*(p + 4) = height;
	*(unsigned short *)buffer =  0xaa55;
	*(unsigned int *)(buffer + 2) = pixel_format;
	*(unsigned short *)(buffer + 6) = width;
	*(unsigned short *)(buffer + 8)= height;
	return sock;
}

void client_deinit(int sockfd)
{
	close(sockfd);
}

int send_yuv420p_data(int sockfd, void *buffer, unsigned int size)
{
	return write(sockfd, buffer, size);
}

