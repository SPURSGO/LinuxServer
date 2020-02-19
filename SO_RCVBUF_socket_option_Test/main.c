#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>

/*
 * SO_SNDBUF和SO_RCVBUF选项分别表示TCP发送缓冲区和TCP接收缓冲区的大小.
 * 当用setsockopt()来设置TCP发送/接收缓冲区大小时，系统都会将其值加倍，并且不低于某个最小值!!!
 * 一般：TCP接收缓冲区最小值256B，TCP发送缓冲区最小值2048B。
 * 这样做的目的：确保一个TCP连接拥有足够的空闲缓冲区来处理拥塞.
 *
 * 此服务器程序测试socket选项SO_RCVBUF以修改内核TCP接收缓冲区大小
 * 注：对于每一条TCP连接，内核都会为该连接维护一系列资源，如：必要数据结构、内核TCP发送/接收缓冲区、连接状态、定时器等
 *
 */

#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
    if(argc <= 2)
    {
        printf("Usage: %s  ip_address  port_number  receive_buffer_size\n",basename(argv[0]));
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in addr;
    bzero(&addr, sizeof (addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addr.sin_addr);
    addr.sin_port = htons((unsigned short int)port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);

    int rcvbuf_size = atoi(argv[3]); //指定要修改的缓冲区的大小(即SO_SNDBUF选项的值)
    int len = sizeof (rcvbuf_size);

    /* 先设置TCP接收缓冲区的大小，然后立即读取其大小 */
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf_size, sizeof (len));
    getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf_size, (socklen_t *)&len);
    printf("the tcp receive buffer size after setting is %d\n",rcvbuf_size);

    int ret = bind(sockfd, (struct sockaddr *)&addr, sizeof (addr));
    assert(ret != -1);

    ret = listen(sockfd, 5);
    assert(ret != -1);

    struct sockaddr_in client;
    int client_addrlen = sizeof (client);
    int connfd = accept(sockfd, (struct sockaddr *)&client, (socklen_t *)&client_addrlen);

    if(connfd < 0)
    {
        printf("connect fail. errno is:%d\n", errno);
    }
    else
    {
        char rcv_buffer[BUFFER_SIZE];
        memset(rcv_buffer, '\0', BUFFER_SIZE);
        while (recv(connfd, rcv_buffer, BUFFER_SIZE-1, 0)) { printf("data received from client:%s\n",rcv_buffer);}
        close(connfd);
    }

    close(sockfd);
    return 0;
}




