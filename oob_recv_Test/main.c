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
 * 此程序是接收OOB数据的服务器程序
 *
 */

#define BUF_SIZE 1024

int main(int argc, char *argv[])
{
    if(argc <= 2)
    {
        printf("usage: %s  ip_address  port_number\n",basename(argv[0])); // <libgen.h>
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]); // <stdlib.h>

    struct sockaddr_in addr;
    bzero(&addr,sizeof (addr)); // <string.h>
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addr.sin_addr);// <arpa/inet.h>
    addr.sin_port = htons((unsigned short int)port);

    /*@@@@@@@@@@@@@@@@@ THINK：实时检测一个函数调用的返回值是一个比较安全的做法 @@@@@@@@@@@@@@@*/
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0); // <assert.h>

    int bind_ret = bind(sockfd, (struct sockaddr *)&addr, sizeof (addr));
    assert(bind_ret != -1);

    int listen_ret = listen(sockfd,5);
    assert(listen_ret != -1);

    struct sockaddr_in client;
    socklen_t client_addrlen = sizeof (client);  // 可以用socklen_t类型指定socket地址变量的大小
    int connfd = accept(sockfd, (struct sockaddr *)&client, &client_addrlen);
    if(connfd < 0)
    {
        printf("connect fail! errno is :%d\n",errno);
    }
    else
    {
        char recv_buf[ BUF_SIZE ];

        memset(recv_buf, '\0', BUF_SIZE);  // memset与bzero的作用相似
        ssize_t recv_ret = recv(connfd, recv_buf, BUF_SIZE-1, 0);
        printf("got %zu bytes of normal data '%s'\n", recv_ret, recv_buf);// 类型的格式化输出参考百度(ssize_t-->%zu)

        memset(recv_buf, '\0', BUF_SIZE);
        recv_ret = recv(connfd, recv_buf, BUF_SIZE-1, MSG_OOB);//指定接收带外数据
        printf("got %zu bytes of oob data '%s'\n", recv_ret, recv_buf);

        memset(recv_buf, '\0', BUF_SIZE);
        recv_ret = recv(connfd, recv_buf, BUF_SIZE-1, 0);
        printf("got %zu bytes of normal data '%s'\n", recv_ret, recv_buf);
    }

    close(sockfd); // <unistd.h>

    return 0;
}
