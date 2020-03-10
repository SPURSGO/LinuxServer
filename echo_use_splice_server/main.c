#define _GNU_SOURCE 1  /* 使用splice()必须加的一个宏定义 */
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>

int main(int argc, char *argv[])
{
    if(argc < 3)
    {
        printf("Usage:%s ipaddr port\n",basename(argv[0]));
        return -1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in addr;
    memset(&addr, '\0', sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short) port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    assert( bind(listenfd, (struct sockaddr *)&addr, sizeof (addr)) != -1 );
    assert( listen(listenfd, 10) != -1 );

    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof (client_addr);
    int connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addrlen);

    if(connfd < 0)
    {
        printf("connect fail and errno is %d\n", errno);
    }
    else
    {
        int pipefd[2];
        assert( pipe(pipefd) != -1 );

        int ret = (int)splice(connfd, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
        assert( ret != -1 );

        ret = (int)splice(pipefd[0], NULL, connfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
        assert( ret != -1 );

        /* 必须在这个else分支里面关闭connfd,因为只有在执行这个分支的时候，才能证明连接是成功的,connfd才是一个socket fd */
        close(connfd);
    }

    /* close(connfd); 不能在此关闭connfd,因为accept可能会失败返回-1，
     * 即connfd==-1,导致close调用失败,因为此时connfd不是一个文件描述符
     */
    close(listenfd);
    return 0;
}
