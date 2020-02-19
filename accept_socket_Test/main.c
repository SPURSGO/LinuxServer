#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>

/*
 * 考虑如下情况：
 * 如果监听队列中处于ESTABLISHED状态的连接对应的客户端出现网络异常(不如掉线)，或者提前推出，
 * 那么服务器对这个连接所执行的accpet()调用是否成功？
 * ----------------------------------------------------------------------------
 * 此服务器程序将测试上述情况
 *
 */

int main(int argc, char *argv[])
{
    if( argc <= 2)
    {
        printf("usage: %s  ip_address  port_number\n",basename(argv[0]));  /* basename() in <libgen.h> */
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);  /* atoi() in <stdlib.h> */

    /* create socket address. */
    struct sockaddr_in address;  /* struct sockaddr_in in <netinet/in.h> */
    bzero(&address,sizeof (address));  /* bzero() in <string.h> */
    address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,(struct sockaddr *)&address.sin_addr); /* int inet_pton(int af,const char *src,void *dst) in <arpa/inet.h> */
    address.sin_port = htons((unsigned short int)port); /* unsigned short int htons(unsigned short int hostshort) in <netinet/in.h> */

    /* PF_INET <---> AF_INET 地址族和协议族是想对应的，且PF_*与AF_*的宏定义的值都完全相同，所以可以相互混用 */
    int sockfd = socket(PF_INET,SOCK_STREAM,0); /* socket() success:return socket fd | fail:return -1 and set errno */
    assert(sockfd >= 0);

    int ret = bind(sockfd,(struct sockaddr *)&address,sizeof (address));  /* bind() success:0 | fial:-1 and set errno */
    assert(ret != -1);

    ret = listen(sockfd,5);  /* listen() success:0 | fial:-1 and set errno */
    assert(ret != -1);

    /* 暂停15秒以等待客户端连接和相关操作(掉线/推出)完成 */
    sleep(15);  /* sleep() in <unistd.h> */

    struct sockaddr_in client;
    socklen_t client_addrlen = sizeof (client);
    int connfd = accept(sockfd,(struct sockaddr *)&client,&client_addrlen); /* accept() success:new socketfd | fail:-1 and set errno*/

    if(connfd < 0 )
    {
        printf("errno is: %d\n", errno); /* errno in <errno.h> */
    }
    else
    {
        /* if accept() called successfully,print client's ip and port. */
        char remote[INET_ADDRSTRLEN]; /* INET_ADDRSTRLEN  16  in <netinet/in.h> */
        /* 注意此处：端口号的输出要提前将其网络字节序表示转化为主机字节序 */
        printf("connected with ip: %s and port %d\n",inet_ntop(AF_INET,&client.sin_addr,remote,INET_ADDRSTRLEN),ntohs(client.sin_port));
        close(connfd);
    }

    close(sockfd);

    return 0;
}







