#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

#define USER_LIMIT      5
#define BUF_SIZE        1024
#define FD_LIMIT        65535
#define MAX_EVENTS      1024
#define PROCESS_LIMIT   65535


/* 处理一个客户连接必要的数据 */
struct client_data
{
    struct sockaddr_in client_addr;  // 保存客户socket地址
    int connfd; // 保存与客户连接的连接套接字
    pid_t pid;  // 处理这个连接的子进程pid
    int pipefd[2];  // 与父进程通信的管道
};


static  int sig_pipefd[2];
static bool stop_child = false;

int setnonblocking(int fd); /* 将文件描述符设置为非阻塞的 */
void epoll_addfd(int epollfd, int fd); /* 将fd上的事件注册到epoll内核事件表 */
void addsig(int sig, void (*handler)(int),  bool restart); /*添加信号*/
void sig_handler(int sig); /* 信号处理 */
int run_child(int idx, client_data *users, char *share_mem);
void child_term_handler(int);

int main( int argc, char *argv[] )
{
    if(argc < 3)
    {
        printf("Usage: %s ip_address port_number\n", basename(argv[0]));
        return -1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in addr;
    memset(&addr, '\0', sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((static_cast<unsigned short>(port)));
    inet_pton(AF_INET, ip, &addr.sin_addr);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    assert(bind(listenfd, (struct sockaddr *)&addr, sizeof (addr)) != -1);
    assert(listen(listenfd, 5) != -1);


    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    epoll_addfd(epollfd, listenfd);  /* 将监听listenfd上的可读事件 */


    int sig_pipefd[2];
    assert(socketpair(PF_UNIX, SOCK_STREAM, 0, sig_pipefd) != -1);  // 创建一个双向管道
    setnonblocking(sig_pipefd[1]);
    epoll_addfd(epollfd, sig_pipefd[0]); /* 将监听sig_pipefd[0]上的可读事件 */

    addsig( SIGCHLD, sig_handler, true);
    addsig( SIGTERM, sig_handler, true);
    addsig( SIGINT, sig_handler, true);
    addsig( SIGPIPE, SIG_IGN, true);

    /* 创建共享内存，作为所有客户socket连接的读缓存 */
    static char const *shm_name = "/my_shm";   // 只有const常量指针才能指向static字符串字面量
    int shmfd = shm_open( shm_name , O_CREAT | O_RDWR, 0666);  //shm_open成功时返回一个fd
    assert(shmfd != -1);
    assert( ftruncate(shmfd, USER_LIMIT*BUF_SIZE) != -1); //ftruncate会将参数fd指定的文件大小改为参数length指定的大小。

    char *share_mem =  /*分配一段空闲内存, 空间大小为:USER_LIMIT*BUF_SIZE */
            static_cast<char *>(mmap(nullptr, USER_LIMIT*BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0));
    assert(share_mem != MAP_FAILED);
    close(shmfd);


    struct epoll_event events[MAX_EVENTS]; /* 用于epoll_wait的输出(即内核反映的一系列就绪事件) */
    int user_count = 0; /* 当前客户数量 */
    /* 客户连接数组。进程用客户连接的编号来索引这个数组，即可取得相关的客户连接数据 */
    client_data* users = new client_data[USER_LIMIT+1];
    /* 子进程和客户连接的映射关系表。用进程的PID来索引这个数组，即可取得该进程所处理的客户连接的编号 */
    int* sub_process = new int[PROCESS_LIMIT];
    for(int i = 0; i < PROCESS_LIMIT; ++i)
    {
        sub_process[i] = -1;
    }

    bool stop_server = false;  /* 控制服务器运行 */
    bool terminate = false;

    while (!stop_server)
    {
        int number = epoll_wait(epollfd, events, MAX_EVENTS, -1);  /* -1，永久阻塞，直到事件发生 */
        if( (number < 0) && (errno != EINTR) )  // 在epoll_wait等待期间，如果程序接收到信号，则其返回-1且设置errno为EINTR
        {
            printf("epoll failure!\n");
            break;
        }

        for(int i = 0; i < number; ++i)  //依次处理epoll_wait输出的就绪事件
        {
            int sockfd = events[i].data.fd;
            if(sockfd == listenfd) /* 表示:listenfd上的可读事件发生，新的客户连接到来 */
            {
                struct sockaddr_in client_addr;
                socklen_t client_addrlen = sizeof(client_addr);

                int connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addrlen); /* 接受客户连接 */

                if(connfd < 0)   // server-client连接失败
                {
                    printf("connect fail! errno is %d\n", errno);
                    continue;  //退出此次循环
                }
                if(user_count >= USER_LIMIT)  //若用户过多，则给客户端发送一个错误信息
                {
                    const char *info = "Too many users.\n";
                    printf("%s", info);
                    send(connfd, info, strlen(info), 0);
                    close(connfd);
                    continue;  //退出此次循环
                }

                /* 保存第user_count个客户连接的相关数据 */
                users[user_count].client_addr = client_addr;
                users[user_count].connfd = connfd;
                /* 在主进程和子进程之间建立双向管道，以传递必要的数据 */
                assert( socketpair(PF_UNIX, SOCK_STREAM, 0, users[user_count].pipefd) != -1 );

                pid_t pid = fork();
                if( pid < 0)
                {
                    printf("fork fail!\n");
                    close(connfd);
                    continue;   //退出此次循环
                }
                else if(pid == 0)
                {   /* 子进程首先关闭由父进程打开的一些文件描述符 */
                    close(epollfd);
                    close(listenfd);
                    close(users[user_count].pipefd[0]); // 子进程关闭由socketpair创建的双向管道fd[0]
                    close(sig_pipefd[0]);
                    close(sig_pipefd[1]);
                    run_child(user_count, users, share_mem);
                    munmap( share_mem, USER_LIMIT*BUF_SIZE );
                    exit(0);
                }
                else
                {
                    close(connfd);
                    close(users[user_count].pipefd[1]); // 父进程关闭由socketpair创建的双向管道fd[1]
                    epoll_addfd(epollfd, users[user_count].pipefd[0]);
                    sub_process[pid] = user_count;
                    user_count++;
                }
            }
            else if( (sockfd ==sig_pipefd[0]) && (events[i].events & EPOLLIN) )/* 处理信号事件 */
            {
                int sig;
                char signals[1024];
                int ret = static_cast<int>(recv(sig_pipefd[0], signals, sizeof (signals), 0));
                if(ret == -1)  // recv()返回-1，表示调用出错
                {
                    continue;
                }
                else if(ret == 0) // recv（）返回0, 表示通信对方已经关闭连接了
                {
                    continue;
                }
                else
                {
                    for(int i = 0; i < ret; ++i)
                    {
                        switch (signals[i])
                        {
                            case SIGCHLD:/* 子进程退出，表示某个客户端关闭了连接 */
                            {
                                pid_t pid;
                                int stat;
                                while( ( pid = waitpid(-1, &stat, WNOHANG) ) > 0 )  // 等待任何一个进程,WNOHANG指定函数立即返回，
                                {                                                   // 再一次体现出非阻塞io和i/o复用的配合
                                    int del_user = sub_process[pid]; /* 用子进程Pid取得被关闭的客户连接的编号 */
                                    sub_process[pid] = -1;
                                    if( (del_user < 0) || (del_user > USER_LIMIT) )
                                    {
                                        continue;
                                    }
                                    /* 清除第del_user个客户连接使用的相关数据 */
                                    epoll_ctl(epollfd, EPOLL_CTL_DEL, users[user_count].pipefd[0], nullptr);
                                    close(users[user_count].pipefd[0]);
                                    users[del_user] = users[--user_count];  // 让最后一个补上被删除的del_user个客户连接,并修改当前用户数量
                                    sub_process[users[del_user].pid] = del_user;
                                }
                                if( terminate && user_count == 0 )
                                {
                                    stop_server = true;
                                }
                                break;
                            }
                            case SIGTERM:
                            case SIGINT:
                            {
                                /* 结束服务器程序 */
                                printf("kill all the child now!\n");
                                if(user_count == 0)
                                {
                                    stop_server = true;
                                    break;
                                }
                                for(int i = 0; i < user_count; ++i)
                                {
                                    int pid = users[i].pid;
                                    kill(pid, SIGTERM);
                                }
                                terminate = true;
                                break;
                            }
                            default:
                            {
                                break;
                            }
                        }
                    }
                }
            }
            else if( events[i].events & EPOLLIN )  /* 某个子进程向父进程写入了数据 */
            {
                int child = 0;
                /* 读取管道数据，child变量记录了是哪个客户连接有数据到达 */
                int ret = static_cast<int>(recv( sockfd, (char *)&child, sizeof (child), 0 ) );
                if(ret == -1)
                {
                    continue;
                }
                else if(ret == 0)
                {
                    continue;
                }
                else
                {
                    //向除负责处理第child个客户连接的子进程之外的其他子进程发送消息，通知他们有客户数据要写
                    for(int j = 0; j < user_count; ++j)
                    {
                        if(users[j].pipefd[0] != sockfd)
                        {
                            printf("send data to child accross pipe!\n");
                            send(users[j].pipefd[0], (char *)&child, sizeof (child), 0);
                        }
                    }
                }
            }
        }
    }

    close(sig_pipefd[0]);
    close(sig_pipefd[1]);
    close(listenfd);
    close(epollfd);
    shm_unlink(shm_name);
    delete [] users;
    delete [] sub_process;

    return 0;
}


int setnonblocking(int fd)
{
    int old_option = fcntl( fd, F_GETFL );
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd, F_SETFL, new_option );
    return old_option;  //返回设置阻塞之前的文件描述符状态，以便恢复
}


void epoll_addfd(int epollfd, int fd)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;   /* 监听可读事件，并改为ET工作模式 */
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}


void addsig(int sig, void (*handler)(int), bool restart = true)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof (sa));
    sa.sa_handler = handler;
    if(restart)
    {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}


void sig_handler(int sig)
{
    int save_errno = errno;
    int msg = sig;
    send(sig_pipefd[1], (char *)(&msg), 1, 0);
    errno = save_errno;
}


void child_term_handler(int)
{
    stop_child = true;
}


/* 子进程运行的函数，参数idx指出该子进程处理的客户连接的编号，users是保存所有客户连接数据的数组，参数share_mem指出共享内存的起始地址 */
int run_child(int idx, client_data *users, char *share_mem)
{
    bool stop_child = false;

    /* 子进程使用I/O复用技术来同时监听两个文件描述符：客户连接socket,与父进程通信的管道文件描述符 */
    int child_epollfd = epoll_create(5);
    assert(child_epollfd != -1);
    int connfd = users[idx].connfd;
    epoll_addfd( child_epollfd, connfd );
    int pipefd = users[idx].pipefd[1];
    epoll_addfd(child_epollfd, pipefd);

    /* 子进程需要设置自己的信号处理函数 */
    addsig(SIGTERM, child_term_handler, false);

    epoll_event events[MAX_EVENTS];  /* 用于接收epoll_wait的输出 */
    while ( !stop_child )
    {
        int number = epoll_wait(child_epollfd, events, MAX_EVENTS, -1);  // 永久阻塞，直到就绪事件到来
        if( (number < 0) && ( errno != EINTR ) )
        {
            printf("epoll failure!\n");
            break;
        }
        for(int i = 0; i < number; ++i)
        {
            int sockfd = events[i].data.fd;
            /* 本子进程负责的客户连接有数据到达 */
            if( (sockfd == connfd) && (events[i].events & EPOLLIN) )
            {
                memset(share_mem + idx*BUF_SIZE, '\0', BUF_SIZE);
                /* 将客户数据读取到对应的读缓冲区中，该读缓冲区是共享内存的一段，它开始于idx*BUF_SIZE处 ,
                 * 长度为BUF——SIZE字节。因此，各个客户连接的读缓存是共享的
                 */
                int ret = static_cast<int>(recv( connfd, share_mem + idx*BUF_SIZE, BUF_SIZE-1,0 )) ;
                if( ret < 0 )
                {
                    if(errno != EAGAIN)
                    {
                        stop_child = true;
                    }
                }
                else if(ret == 0)
                {
                    stop_child = true;
                }
                else
                {
                    /* 成功读取客户数据后通知主进程(通过管道)来处理 */
                    send(pipefd, (char *)&idx, sizeof (idx), 0);

                }
            }
            /* 主进程通知本进程(通过管道)将第client个客户的数据发送到本进程负责的客户端 */
            else if((sockfd == pipefd) && (events[i].events & EPOLLIN))
            {
                int client = 0;
                /* 接收主进程发送来的数据，即有客户连接到达的连接的编号 */
                int ret = static_cast<int>(recv(sockfd, (char *)&client, sizeof (client), 0));
                if(ret < 0 )
                {
                    if(errno != EAGAIN)
                    {
                        stop_child = true;
                    }
                }
                else if(ret == 0)
                {
                    stop_child = true;
                }
                else
                {
                    send( connfd, share_mem + client*BUF_SIZE, BUF_SIZE, 0 );
                }
            }
            else
            {
                continue;
            }
        }
    }

    close(connfd);
    close(pipefd);
    close(child_epollfd);
    return 0;
}









