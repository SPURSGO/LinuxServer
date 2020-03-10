#define _GNU_SOURCE 1
#include <iostream>
#include <libgen.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>

using namespace std;

#define BUF_SIZE 64

int main( int argc, char *argv[] )
{
    if(argc < 3)
    {
        printf("Usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    const char *ip = argv[1];
    int port = static_cast<unsigned short>(atoi(argv[2]));

    sockaddr_in serv_addr;
    memset(&serv_addr, '\0', sizeof (serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(static_cast<unsigned short>(port) );
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert( sockfd >= 0 );

    if( connect(sockfd, (sockaddr *)&serv_addr, sizeof (serv_addr)) < 0 )
    {
        cout << "connection failure!" << endl;
        close(sockfd);
        return 1;
    }

    pollfd fds[2];
    /* 注册文件描述符0(标准输入)和文件描述符sockfd上的可读事件 */
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    fds[1].fd = sockfd;
    fds[1].events = POLLIN | POLLRDHUP;  //POLLRDHUP表示：TCP连接被对方关闭，或者对方关闭了写操作。它由GNU引入
    fds[1].revents = 0;

    char read_buf[BUF_SIZE];
    int pipefd[2];
    assert( pipe(pipefd) != -1 );  /* 创建管道文件描述符, 往管道写端pipefd[1]中写入的数据可以从读端pipefd[0]读出 */

    while (1)
    {
        int ret = poll(fds, 2, -1);  /* timeout = -1,poll将永久阻塞，直到某个事件发生 */
        if(ret < 0)
        {
            printf("poll failure!\n");
            break;
        }

        if(fds[1].revents & POLLRDHUP)
        {
            printf("Server close the connection!\n");
            break;
        }
        else if(fds[1].revents & POLLIN)
        {
            memset(read_buf, '\0', BUF_SIZE);
            recv(fds[1].fd, read_buf, BUF_SIZE-1, 0);
            printf("%s\n", read_buf);
        }

        if(fds[0].revents & POLLIN)
        {
            /* 使用splice（）将用户输入的数据直接写到sockfd上(零拷贝),在使用splice()时，fd_in/fd_out必须至少有一个是管道文件描述符 */
            /* 首先：标准输入--->写端管道*/
            /* 其次：读端管道--->连接套接字 */
            ret = static_cast<int>( splice(0, nullptr, pipefd[1], nullptr, 300, SPLICE_F_MORE | SPLICE_F_MOVE) );
            ret = static_cast<int>( splice(pipefd[0], nullptr, sockfd, nullptr, 300, SPLICE_F_MORE | SPLICE_F_MOVE) );
        }
    }

    close(sockfd);
    return 0;
}
