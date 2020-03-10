#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <assert.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#define MAX_EVENTS 1024
#define TCP_BUFSIZE 512
#define UDP_BUFSIZE 1024

int setnonblock(int fd);
void add_epollin_events(int epollfd, int fd);


int main( int argc, char *argv[] )
{
    if(argc < 3)
    {
        printf("usage: %s ip port\n", basename(argv[0]));
        return -1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    sockaddr_in addr;
    memset(&addr, '\0', sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<unsigned short>(port));
    inet_pton(AF_INET, ip, &addr.sin_addr);

    /* 创建TCP socket, 并将其绑定到端口port上 */
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert( listenfd != -1 );

    assert( bind(listenfd, reinterpret_cast<sockaddr *>(&addr), sizeof (addr)) != -1 );
    assert( listen(listenfd, 10) != -1 );


    /* 创建UDP socket,并将其绑定到端口port上 */
    memset(&addr, '\0', sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<unsigned short>(port));
    inet_pton(AF_INET, ip, &addr.sin_addr);

    int udpfd = socket(PF_INET, SOCK_DGRAM, 0);
    assert(udpfd >= 0);
    assert( bind(udpfd, reinterpret_cast<sockaddr *>(&addr), sizeof (addr)) != -1 );

    int epollfd = epoll_create(10);
    assert(epollfd >= 0);

    /* 注册TCP socket 和 UDP socket 上的可读事件 */
    add_epollin_events(epollfd, listenfd);
    add_epollin_events(epollfd, udpfd);

    epoll_event events[MAX_EVENTS]; /* 用于epoll_wait的输出 */
    while (1)
    {
        int ret_number = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if(ret_number < 0)
        {
            printf("epoll fail!\n");
            break;
        }

        for(int i = 0; i < ret_number; ++i)
        {
            int sockfd = events[i].data.fd;
            if(sockfd == listenfd)
            {
                sockaddr_in client_addr;
                socklen_t client_addrlen = sizeof(client_addr);
                int connfd = accept(listenfd, reinterpret_cast<sockaddr *>(&client_addr), &client_addrlen);
                add_epollin_events(epollfd, connfd);
            }
            else if(sockfd == udpfd)
            {
                char buf[UDP_BUFSIZE]; /* UDP用户接收缓冲区 */
                memset(buf, '\0', UDP_BUFSIZE);
                sockaddr_in client_addr;
                socklen_t client_addrlen = sizeof (client_addr);
                int ret = static_cast<int>( recvfrom(udpfd, buf, UDP_BUFSIZE-1, 0,
                                                    reinterpret_cast<sockaddr *>(&client_addr), &client_addrlen) );
                if(ret > 0)
                {
                    sendto(udpfd, buf, UDP_BUFSIZE, 0, reinterpret_cast<sockaddr *>(&client_addr), client_addrlen);
                }
            }
            else if(events[i].events & EPOLLIN) /* ET模式：立即将就绪事件一次性处理完毕 */
            {
                char buf[TCP_BUFSIZE];  /* TCP用户接收缓冲区 */
                while(1)
                {
                    memset(buf, '\0', TCP_BUFSIZE);
                    int ret = static_cast<int>(recv(sockfd, buf, TCP_BUFSIZE, 0));
                    if(ret < 0)
                    {
                        if( (errno == EAGAIN) || (errno == EWOULDBLOCK) ) /* 暂时没有数据可读，说明此次可读数据已全部读完 */
                        {
                            break;
                        }
                        close(sockfd);
                        break;
                    }
                    else if( ret == 0)
                    {
                        close(sockfd);
                    }
                    else
                    {
                        send(sockfd, buf, TCP_BUFSIZE, 0);
                    }
                }
            }
            else
            {
                printf("something else happened!\n");
            }
        }
    }
    close(listenfd);
    return 0;
}


int setnonblock(int fd)
{
    int old_opt = fcntl(fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_opt);
    return old_opt;
}


void add_epollin_events(int epollfd, int fd)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    /* 注意，每个使用ET模式的文件描述符都应该是非阻塞的. */
    /* 如果fd是阻塞的，那么读写操作将会因为没有后续的事件而一直处于阻塞状态 */
    setnonblock(fd);
}


