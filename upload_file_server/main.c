#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[])
{
    if(argc < 3)
    {
        printf("Usage: %s ip_address port_number\n",basename(argv[0]));
        printf("Example: %s 192.168.1.11 12345\n",basename(argv[0]));
        exit(1);
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in addr;
    bzero(&addr, sizeof (addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addr.sin_addr);
    addr.sin_port = htons((unsigned short)port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);

    int ret = bind(sockfd, (struct sockaddr *)&addr, sizeof (addr));
    assert(ret != -1);

    ret = listen(sockfd,10);
    assert(ret != -1);

    struct sockaddr_in client;
    int client_addrlen = sizeof (client);
    int connfd = accept(sockfd, (struct sockaddr *)&client, (socklen_t *)&client_addrlen);
    if(connfd < 0)
    {
        printf("connect fail!\n");
        exit(1);
    }
    else
    {
        printf("Receive some data from client computer!\n");
        char client_addr[INET_ADDRSTRLEN];
        printf("Client's ip address is %s\n",inet_ntop(AF_INET, &client.sin_addr, client_addr, INET_ADDRSTRLEN));
    }

    char filename_buf[BUF_SIZE];
    memset(filename_buf, '\0',BUF_SIZE);
    ssize_t recv_ret;  //记录成功读取到的字节数
    if((recv_ret=recv(connfd, filename_buf, BUF_SIZE-1, 0)) < 0)
    {
        printf("read filename failed!\n");
    }
    else
    {
        printf("successfully receive filename(%zu bytes).\n",recv_ret);
    }


    FILE *fp = fopen(filename_buf,"w");
    if(fp == NULL)
    {
        printf("create file fail.\n");
        exit(1);
    }
    else
    {
        printf("create file successfully.\n");
    }

    char recv_buf[BUF_SIZE];  // 可以调用memset/bzero
    memset(recv_buf, '\0', BUF_SIZE);
    while ((recv(connfd, recv_buf, BUF_SIZE, 0)) > 0)
    {
        fprintf(fp, "%s",recv_buf);
        //每次接收完数据并且读到文件中之后，都要清空接收缓冲区!!!!!!!!!!!!!!!
        //这一点非常重要，否则该接收缓冲区中将存在上一次接收的数据，所以造成数据重复与错误.
        memset(recv_buf, '\0', BUF_SIZE);
    }

    fclose(fp);
    close(connfd);
    close(sockfd);

    return 0;
}
