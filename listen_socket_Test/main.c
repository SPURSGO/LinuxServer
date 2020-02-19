#include <stdio.h>
#include <signal.h>         /*   --->signal()               */
#include <libgen.h>         /*   --->basename()             */
#include <stdlib.h>         /*   --->atoi()                 */
#include <sys/socket.h>     /*   --->socket()               */
#include <assert.h>         /*   --->assert()               */
#include <netinet/in.h>     /*   --->struct sockaddr_in     */
#include <string.h>         /*   --->bzero()                */
#include <arpa/inet.h>      /*   --->inet_pton()            */
#include <unistd.h>         /*   --->sleep()                */


enum bool{ false,true };  //枚举类型bool
static enum bool stop = false;


/* SIGTERM信号的处理函数，触发时结束主程序中的循环*/
static void handle_term( /* int sig */ )
{
    stop = true;
}


int main(int argc, char* argv[])
{
    signal(SIGTERM, handle_term);

    if( argc <= 3)
    {
        /* basename函数可以返回路径最后一个路径分隔符之后的内容，比如basename("/usr/local/abc") 返回 abc. */
        printf("usage: ./%s ip_address  port_number  backlog\n",basename(argv[0]));
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    int backlog = atoi(argv[3]);

    int sock = socket(PF_INET,SOCK_STREAM,0); /* socket(),PF_INET,SOCK_STREAM in <sys/socket.h> */
    assert(sock >= 0);

    /* 构造一个IPv4的socket地址(套接字) */
    struct sockaddr_in address;
    bzero(&address,sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&address.sin_addr); /* inet_pton() in <arpa/inet.h> */
    address.sin_port = htons((unsigned short int)port); /* htons() in #include <netinet/in.h> */

    int ret = bind(sock,(struct sockaddr *)&address,sizeof (address)); /* bind() in <sys/socket.h> */
    assert(ret != -1);

    ret = listen(sock,backlog);
    assert(ret != -1);

    /* 循环等待连接，直到有SIGTERM信号将它中断 */
    while (!stop) {
        sleep(1);
    }

    /* close socket */
    close(sock);

    return 0;
}










