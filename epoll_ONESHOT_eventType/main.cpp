#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_EVENTS 1024
#define BUF_SIZE 1024

struct fds
{
    int epollfd;
    int sockfd;
};


int setnonblocking(int fd);
void add_epollin_event(int epollfd, int fd, bool oneshot);
void reset_oneshot(int epollfd, int fd);
void* worker(void *arg);  /* 注意此处的参数和返回值都是void *类型的 */

int main( int argc, char *argv[] )
{
    if( argc < 3 )
    {
        printf("usage: %s ip port\n",basename(argv[0]));
        return -1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    sockaddr_in addr;
    memset(&addr, '\0', sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<unsigned short>(port));
    inet_pton(AF_INET, ip, &addr.sin_addr);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd != -1);

    assert( bind(listenfd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != -1 );
    assert( listen(listenfd, 10) != -1 );

    int epollfd = epoll_create(10);
    assert(epollfd != -1);

    /* 注意：监听socket上不能注册EPOLLONESHOT事件 */
    add_epollin_event(epollfd, listenfd, false);

    epoll_event events[MAX_EVENTS];  /* 用于epoll_wait的输出 */
    while (1)
    {
        int ret = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if(ret < 0)
        {
            printf("epoll fail!\n");
            break;
        }

        for(int i = 0; i < ret; ++i)
        {
            int sockfd = events[i].data.fd;
            if(sockfd == listenfd)
            {
                sockaddr_in client_addr;
                socklen_t client_addrlen = sizeof (client_addr);
                int connfd = accept(listenfd, reinterpret_cast<sockaddr *>(&client_addr), &client_addrlen);
                /* 对每个非监听文件描述符都注册EPOLLONESHOT事件 */
                add_epollin_event(epollfd, connfd, true);
            }
            else if(events[i].events & EPOLLIN)
            {
                pthread_t thread; /* typedef unsigned long int pthread_t; */
                fds fds_for_new_works;
                fds_for_new_works.epollfd = epollfd;
                fds_for_new_works.sockfd = sockfd;
                /* 新启动一个工作线程为sockfd服务(sockfd == connfd) */
                pthread_create( &thread, nullptr, worker, reinterpret_cast<void *>(&fds_for_new_works) );
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


int setnonblocking(int fd)
{
    int old_opt = fcntl(fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_opt);
    return old_opt;
}


void add_epollin_event(int epollfd, int fd, bool oneshot)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    if(oneshot)
    {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);  /* 注意：采用ET工作模式的fd,必须是非阻塞的 */
}


/* 重置fd上的事件。这样操作之后，尽管fd上的EPOLLONESHOT事件被注册，
 * 但是操作系统仍然会触发fd上的EPOLLIN事件，且只触发一次
 */
void reset_oneshot(int epollfd, int fd)
{
    epoll_event event;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    event.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
}

/* 工作线程 */
void* worker(void *arg)
{
    int sockfd = reinterpret_cast<fds *>(arg)->sockfd;  // connfd == sockfd
    int epollfd = reinterpret_cast<fds *>(arg)->epollfd;
    printf("strat new thread to receive data on fd:%d\n",sockfd);

    char buf[BUF_SIZE];


    /* 循环读取sockfd上的数据，直到遇到EAGAIN错误 */
    while(1) /* 注意，在connfd上采用了ET工作模式，所以在处理某一次读就绪事件时，要将可读的数据读完 */
    {
        memset(buf, '\0', BUF_SIZE);
        int ret = static_cast<int>( recv(sockfd, buf, BUF_SIZE-1, 0) );
        if( ret == 0 )
        {
            close( sockfd );
            printf( "foreiner closed the connection\n" );
            break;
        }
        else if( ret < 0 )
        {
            if( errno == EAGAIN )  /* 如果发生EAGAIN错误，说明此次读数据已经读完,可以等待下一次的EPOLLIN事件 */
            {
                reset_oneshot(epollfd, sockfd);
                printf( "read later!\n" );
                break;
            }
        }
        else
        {
            printf( "get content: %s\n", buf);
            /* 休眠10秒，模拟数据处理过程 */
            sleep(10);
        }
    }
    printf("end thread receiving data on fd:%d\n", sockfd);
}




