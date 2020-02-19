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
 * TCP数据读写API测试
 * ------------------------------------
 * ssize_t recv(int sockfd, void *buf, size_t len, int flags);
 * ssize_t send(int sockfd, const void *buf, size_t len, int flags);
 *
 * 本例测试flags标志中的MSG_OOB标志
 * 此程序是发送OOB数据的客户程序
 *
 */


int main(int argc, char *argv[])
{
    if(argc <= 2)
    {
        printf("usage: %s  id_address  port_number\n",basename(argv[0])); // <libgen.h>
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]); // <stdlib.h>

    struct sockaddr_in server_addr;
    bzero(&server_addr,sizeof (server_addr)); // <string.h>
    server_addr.sin_family = AF_INET;
    inet_pton(PF_INET,ip,&server_addr.sin_addr); // <arpa/inet.h>
    server_addr.sin_port = htons((unsigned short int)port);

    int sockfd = socket(PF_INET,SOCK_STREAM,0);
    assert(sockfd >= 0); // <assert.h>

    /* 一定要根据connect()调用的返回值检测调用是否成功(成功返回0，失败返回-1)，否则将无法知道该连接的socket是否可以进行读写通信 */
    if( connect(sockfd, (struct sockaddr*)&server_addr, sizeof (server_addr)) < 0 )
    {
        /* fail */
        printf("connection failed:%d\n",errno); // <errno.h>
    }
    else
    {
        /* success */
        const char *oob_data = "abc";
        const char *normal_data = "123";
        /* 对于字符发送缓冲区的大小，用strlen()指定 */
        send(sockfd, normal_data, strlen(normal_data), 0);
        send(sockfd, oob_data, strlen(oob_data), MSG_OOB);
        send(sockfd, normal_data, strlen(normal_data), 0);
    }

    close(sockfd); // <unistd.h>

    return 0;
}




