#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "video.h"
#include "client.h"

#define HEADER_SIZE 12

int client_init(void)
{
    int sock;
    struct sockaddr_in server_addr;
    char header[HEADER_SIZE];
    char *data;
    int format, width, height;
    size_t data_size;

    // 创建套接字
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 设置服务器地址结构体
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    // 连接服务器
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(sock);
        exit(EXIT_FAILURE);
    }

}

void client_deinit(int sockfd)
{
	close(sockfd);
}

void send_v4l2_data(const char *server_ip, int server_port) {
    // 获取v4l2数据
    data = (char*)get_v4l2_data(&data_size);
    format = get_v4l2_format();
    width = get_v4l2_width();
    height = get_v4l2_height();

    // 打包头部
    header[0] = 0xAA;
    header[1] = 0x55;
    memcpy(header + 2, &format, 4);
    memcpy(header + 6, &width, 2);
    memcpy(header + 8, &height, 2);
    header[10] = 0x55;
    header[11] = 0xAA;

    // 发送头部
    if (send(sock, header, HEADER_SIZE, 0) == -1) {
        perror("send header");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // 发送数据
    if (send(sock, data, data_size, 0) == -1) {
        perror("send data");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Data sent successfully\n");

    // 关闭套接字
    close(sock);
}

