#include <stdio.h>
#include <libgen.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define BUF_SIZE 1024

int main( int argc, char *argv[] )
{
    if(argc < 3)
    {
        printf("Usage: %s ip_address port_number\n", basename(argv[0]));
        return -1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in serv_addr;
    memset(&serv_addr, '\0', sizeof (serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons((unsigned short)port);
    inet_pton(AF_INET, ip, /*(struct sockaddr *)*/&serv_addr);  /* 注意inet_pton的第三个参数(void*)*/

    /* 若将第三个参数进行了类型转换(即转换为通用socket地址结构)，则会引发97号错误，其错误描述如下: */
    // #define	EAFNOSUPPORT	97	 /* Address family not supported by protocol */
    /* 更多错误类型，可见errno.h */

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert( sockfd != -1);

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof (serv_addr)) < 0 )
    {
        printf("connect fail!\n");
        printf("errno is %d\n",errno);
    }
    else
    {
        char read_buf[BUF_SIZE];
        while(1)
        {
            memset(read_buf, '\0', BUF_SIZE);
            int ret = (int)recv(sockfd, read_buf, BUF_SIZE - 1, 0);
            if(ret < 0)
            {
                printf("recv error!\n");
                break;
            }
            else if(ret == 0)
            {
                printf("connection closed!\n");
                break;
            }
            else
            {
                printf("Receive data from server: %s\n", read_buf);
            }
        }
    }
    close(sockfd);

    return 0;
}
