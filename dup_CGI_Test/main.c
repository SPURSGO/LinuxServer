#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

int main( int argc, char *argv[] )
{
    if(argc < 3)
    {
        printf("Usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof (serv_addr));
    serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);
    serv_addr.sin_port = htons((unsigned short)port);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    int ret = bind(sock, (struct sockaddr *)&serv_addr, sizeof (serv_addr));
    assert(ret != -1);

    ret = listen(sock, 5);
    assert(ret != -1);

    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof (client_addr);
    int connfd = accept(sock, (struct sockaddr *)&client_addr, &client_addrlen);
    if(connfd < 0)
    {
        printf("connect fail! errno is %d\n", errno);
    }
    else
    {
        close( STDOUT_FILENO );  /* STDOUT_FILENO, close() ---> <unistd.h> */
        dup(connfd);  /* dup(),dup2() ---> <unistd.h> */
        printf("abcd\n");
        close(connfd);
    }

    close(sock);
    return 0;
}





