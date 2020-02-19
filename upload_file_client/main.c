#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[])
{
    if(argc < 4)
    {
        printf("Usage: %s filename server_ip_address server_port\n",argv[0]);
        printf("Example: %s /root/linux/server/test.txt 192.168.1.11 12345\n",argv[0]);
        return 1;
    }


    FILE *fp;   // 文件指针

    //此处的fopen使用相对路径或者绝对路径都可以,并且路径中无论是否含有中文，均可解析
    if((fp = fopen(argv[1],"r")) == NULL)
    {
        perror("fail to read.\n");
        exit(1);  // exit() in <stdlib.h>:exit(0)表示正常退出,exit(x)（x不为0）都表示异常退出，这个x是返回给操作系统的，以供其他程序使用。
    }

    const char *ip = argv[2];
    int port = atoi(argv[3]);

    struct sockaddr_in server_addr;
    bzero(&server_addr,sizeof (server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_addr.sin_addr);
    server_addr.sin_port = htons((unsigned short)port);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    if(connect(sock, (struct sockaddr *)&server_addr, sizeof (server_addr)) == -1)
    {
        printf("connect fail!\n");
        exit(1);
    }

    ssize_t send_ret; //记录成功发送到的字节数
    //在此处可以考虑指定一个flag值，指明本次数据发送的方式.如：MSG_OOB
    if((send_ret = send(sock, argv[1], strlen(argv[1]), 0)) <= 0) //首先发送本地文件的文件名给服务器端
    {
        printf("send filename error!\n");
        exit(1);
    }
    else
    {
        printf("successfully send local filename(%zu bytes).\n", send_ret);
    }

    printf("正在上传文件......\n");
    sleep(3);  //此处阻塞，让客户先发送一个关于文件名的TCP报文段，同时也可等待服务器程序从其接收缓冲区中读取文件名数据


    char read_buf[BUF_SIZE];  //读取本地文件内容的缓冲区
    unsigned long len; //记录读取的每行字符的个数

    while (fgets(read_buf, BUF_SIZE, fp))
    {
        len = strlen(read_buf);
        /* read_buf[len-1] = '\0';  //去掉每行末尾换行符,这样我们自己可以控制读到的数据的输出格式 */
        /* printf("%s\n",read_buf);  // unsigned long ---> %lu */
        /* 此处，通过TCP连接将文件读取缓冲区的内容发送给服务器端,所以无须客户端自己控制其格式化输出 */
        ssize_t ret;
        if((ret=send(sock, read_buf, len, 0)) <= 0)
        {
            printf("send data error!\n");
            exit(1);
        }
        else
        {
            printf("send %zu bytes data\n",ret);
        }
    }

    close(sock);
    fclose(fp);
    return 0;
}




