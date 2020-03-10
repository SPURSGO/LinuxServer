#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>

int main( int argc, char *argv[] )
{
    if(argc < 3)
    {
        printf("Usage: %s ip_addr port\n", argv[0]);
        return -1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in addr;
    memset(&addr, '\0', sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert( listenfd != -1 );

    assert(bind(listenfd, (struct sockaddr *)&addr, sizeof (addr)) != -1);
    assert( listen(listenfd, 5) != -1 );

    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof (client_addr);
    int connfd = accept( listenfd, (struct sockaddr *)&client_addr, &client_addrlen);
    if(connfd < 0)
    {
        printf("connect fail, errno is %d\n", errno);
        close(listenfd);
        return -1;
    }

    char buf[1024];
    fd_set read_fds;
    fd_set exception_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&exception_fds);

    while (1)  /* 直到数据接收完毕或者关闭socket连接时，就退出循环 */
    {
         memset(buf, '\0', sizeof (buf));
         /* 每次调用select()之前都要重新在read_fds,exception_fds中重置文件描述符connfd */
         /* 因为事件发生后，文件描述符集合将被内核修改 */
         FD_SET(connfd, &read_fds);
         FD_SET(connfd, &exception_fds);
         int ret = select(connfd+1, &read_fds, NULL, &exception_fds, NULL);
         if(ret < 0)
         {
             printf("select fail and errno is %d!\n", errno);
             break;
         }
         /* 对于connfd上的可读事件， */
         if( FD_ISSET(connfd, &read_fds) )
         {
             ret = (int)recv(connfd, buf, sizeof (buf)-1, 0);
             if(ret <= 0)
             {
                 break;
             }
             printf("get %d bytes of normal data: %s\n", ret, buf);
         }
         else if( FD_ISSET(connfd, &exception_fds) )
         {
             ret = (int)recv(connfd, buf, sizeof (buf)-1, MSG_OOB);
             if(ret <= 0)
             {
                 break;
             }
             printf("get %d bytes of oob data: %s\n", ret, buf);
         }
    }

    close(connfd);
    close(listenfd);
    return 0;
}
