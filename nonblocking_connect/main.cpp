#include <stdio.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <libgen.h>
#include <stdlib.h>

#define BUF_SIZE 1024

int setnonblocking(int fd)
{
    int old_opt = fcntl(fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_opt);
    return old_opt;
}


/* 超时连接函数，参数分别是服务器IP地址，端口号和超时时间(秒)
 * 函数成功时返回已经处于连接状态的socket,失败则返回-1
 */
int unblock_connect(const char* ip, int port, int time)
{
    sockaddr_in addr;  /* 服务器的socket地址 */
    memset(&addr, '\0', sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<unsigned short>(port));
    inet_pton(AF_INET, ip, &addr.sin_addr);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    int fd_opt = setnonblocking(sockfd); /* 保存sockfd被设置为非阻塞之前的文件描述符状态 */

    int ret = connect(sockfd, reinterpret_cast<sockaddr *>(&addr), sizeof (addr));
    if(ret == 0)
    {
        /* 如果连接成功，则恢复sockfd的属性，并立即返回之 */
        printf("connect with server immediately!\n");
        fcntl(sockfd, F_SETFL, fd_opt);
        return sockfd;
    }

    else if(errno != EINPROGRESS)
    {
        /* 如果连接没有立即建立，那么只有当errno是EINPROGRESS时才表示连接还在进行，否则出错返回 */
        printf("unblock connect not support!\n");
        return -1;
    }

    fd_set readfds;
    fd_set writefds;
    timeval timeout;

    FD_ZERO( &readfds );
    FD_SET( sockfd, &writefds );

    timeout.tv_sec = time;
    timeout.tv_usec = 0;

    ret = select(sockfd+1, nullptr, &writefds, nullptr, &timeout);
    if(ret <= 0)
    {
        /* select超时或错误，立即返回 */
        printf("connection time out!\n");
        close(sockfd);
        return -1;
    }

    if( ! FD_ISSET(sockfd, &writefds) )
    {
        printf("no events on sockfd found!\n");
        close(sockfd);
        return -1;
    }

    int error = 0; /* 用于接收socket属性的值 */
    socklen_t lenth = sizeof(error);
    /* 调用getsockopt来获取并清除sockfd上的错误 */
    if(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &lenth) < 0)
    {
        printf("get socket option failed!\n");
        close(sockfd);
        return -1;
    }
    /* 若错误号不为0表示连接出错 */
    if(error != 0)
    {
        printf("connect failed after select with the error:%d\n",error);
        close(sockfd);
        return -1;
    }
    /* 连接成功 */
    printf("connection ready after select with the socket:%d\n", sockfd);
    fcntl(sockfd, F_SETFL, fd_opt);
    return sockfd;
}



int main( int argc, char *argv[] )
{
    if(argc < 3)
    {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    int sockfd = unblock_connect(ip, port, 10);
    if(sockfd < 0)
    {
        return 1;
    }

    close(sockfd);
    return 0;
}
