#ifndef CLIENT_H
#define CLIENT_H

#define SZ_HEADER	2
#define SZ_PIXEL_FORMAT 4
#define SZ_WIDTH	2
#define SZ_HEIGHT	2

// 客户端函数声明
void send_v4l2_data(const char *server_ip, int server_port);
void client_deinit(int sockfd);
int client_init(char *server_ip, unsigned short server_port, char *buffer, unsigned int pixel_format, unsigned short width, unsigned short height);
int send_yuv420p_data(int sockfd, void *buffer, unsigned int size);

#endif // CLIENT_H

