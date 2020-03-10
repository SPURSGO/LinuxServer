#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <assert.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>

#define MAX_EVENTS 1024
#define BUF_SIZE   10


void add_epollin_event(int epollfd, int fd, int enable_et); /* enable_et == 1 为启用ET模式，为0则禁用 */
int setnonblocking(int fd);
void work_lt(struct epoll_event *events, int number, int epollfd, int listenfd);
void work_et(struct epoll_event *events, int number, int epollfd, int listenfd);

int main(int argc, char *argv[])
{
    if(argc < 3)
    {
        printf("Usage: %s ip port\n", basename(argv[0]) );
        return -1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in addr;
    memset(&addr, '\0', sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port );
    inet_pton(AF_INET, ip, &addr.sin_addr);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd != -1);

    assert(bind(listenfd, (struct sockaddr *)&addr, sizeof (addr)) != -1);
    assert(listen(listenfd, 10) != -1);

    int epollfd = epoll_create(10);
    assert(epollfd != -1);
    add_epollin_event(epollfd, listenfd, 1);  //将文件描述符listenfd上的可读事件注册到epoll内核事件表上，并设置ET模式

    struct epoll_event events[MAX_EVENTS];
    while(1)
    {
        int ret = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if(ret < 0)
        {
            printf("epoll failure!\n");
            break;
        }
        work_lt(events, ret, epollfd, listenfd);
//        work_et(events, ret, epollfd, listenfd);
    }

    close(listenfd);

    return 0;
}


void add_epollin_event(int epollfd, int fd, int enable_et)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    if(enable_et == 1)
    {
        event.events |= EPOLLET;  /* event.events =  event.events | EPOLLET */
    }

    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}


int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}


void work_lt(struct epoll_event *events, int number, int epollfd, int listenfd)
{
    char buf[BUF_SIZE];
    for(int i = 0; i < number; ++i)
    {
        int sockfd = events[i].data.fd;
        if(sockfd == listenfd)
        {
            struct sockaddr_in client_addr;
            socklen_t client_addrlen = sizeof (client_addr);
            int connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addrlen);
            if(connfd < 0)
            {
                printf("connect fail!\n");
            }
            add_epollin_event(epollfd, connfd, 0); /* 监听了连接套接字上的可读事件，对connfd禁用ET模式 */
        }
        else if (events[i].events & EPOLLIN) /* sockfd == connfd */
        {
            /* 只要socket读缓存中还有未读出的数据，这段代码就被触发 */
            printf("event trigger once!\n");
            memset(buf, '\0', BUF_SIZE);
            int ret = (int)recv(sockfd, buf, BUF_SIZE-1, 0);
            if(ret < 0)
            {
                close(sockfd);
                continue;
            }
            if(ret == 0)
            {
                printf("connect close!\n");
                close(sockfd);
                break;
            }
            printf("get %d bytes of content: %s \n", ret, buf);
        }
        else
        {
            printf("something eles happened\n");
        }
    }
}



void work_et(struct epoll_event *events, int number, int epollfd, int listenfd)
{
    char buf[BUF_SIZE];
    for(int i = 0; i < number; ++i)
    {
        int sockfd = events[i].data.fd;
        if(sockfd == listenfd)
        {
            struct sockaddr_in client_addr;
            socklen_t client_addrlen = sizeof (client_addr);
            int connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addrlen);
            if(connfd < 0)
            {
                printf("connect fail!\n");
            }
            add_epollin_event(epollfd, connfd, 1); /* 监听了连接套接字上的可读事件，对connfd启用ET模式 */
        }
        else if( events[i].events & EPOLLIN )  /* sockfd == connfd */
        {
            /* 这段代码不会被重复触发，所以循环读取数据，以确保把socket读缓存中的所有数据读出 */
            printf("event trigger once!\n");
            while (1)  /* 直到没有数据可读或者断开连接，就结束循环 */
            {
                memset(buf, '\0', BUF_SIZE);
                int ret = (int)recv(sockfd, buf, BUF_SIZE-1, 0);
                if(ret < 0)
                {
                    /* 对于非阻塞IO，下面的条件成立表示数据已经全部读取完毕。*/
                    /* 此后，epoll就能再次触发sockfd上的EPOLLIN事件，以驱动下一次读操作 */
                    if( (errno == EAGAIN) || (errno == EWOULDBLOCK))
                    {
                        printf("read later!\n");
                        break;
                    }
                    close(sockfd);
                    break;
                }
                else if(ret == 0)
                {
                    printf("connect close!\n");
                    close(sockfd);
                }
                else
                {
                    printf("get %d bytes of content: %s\n", ret, buf );
                }
            }
        }
        else
        {
            printf("something else happened!\n");
        }
    }

}

