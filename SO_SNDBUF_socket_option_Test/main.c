#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>

/*
 * SO_SNDBUF和SO_RCVBUF选项分别表示TCP发送缓冲区和TCP接收缓冲区的大小.
 * 当用setsockopt()来设置TCP发送/接收缓冲区大小时，系统都会将其值加倍，并且不低于某个最小值!!!
 * 一般：TCP接收缓冲区最小值256B，TCP发送缓冲区最小值2048B。
 * 这样做的目的：确保一个TCP连接拥有足够的空闲缓冲区来处理拥塞.
 *
 * 此客户程序测试socket选项SO_SNDBUF以修改内核TCP发送缓冲区大小
 * 注：对于每一条TCP连接，内核都会为该连接维护一系列资源，如：必要数据结构、内核TCP发送/接收缓冲区、连接状态、定时器等
 *
 */

#define BUFFER_SIZE 512

int main(int argc, char *argv[])
{
    if(argc <= 2)
    {
        printf("Usage:%s  ip_address  port_number  send_buffer_size.\n",basename(argv[0]));
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in server_addr;
    bzero(&server_addr,sizeof (server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_addr.sin_addr);
    server_addr.sin_port = htons((unsigned short int)port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);

    int sendbuf_size = atoi(argv[3]); //指定要修改的缓冲区的大小(即SO_SNDBUF选项的值)
    int len = sizeof (sendbuf_size);

    /* 先设置TCP发送缓冲区的大小，然后立即读取其大小 */
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sendbuf_size, sizeof(sendbuf_size));
    getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sendbuf_size, (socklen_t *)&len);
    printf("the tcp send buffer size after setting is %d\n",sendbuf_size);

    if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof (server_addr)) != -1)
    {
        char send_buf[BUFFER_SIZE];
        memset(send_buf, 'a', BUFFER_SIZE);
        send(sockfd, send_buf, BUFFER_SIZE-1, 0);
    }

    close(sockfd);
    return 0;
}




