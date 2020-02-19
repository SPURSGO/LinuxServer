#ifndef CALL_INTERFACE_H
#define CALL_INTERFACE_H

#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

/* check the number of arguments. if success,return 0; if fail,return -1 */
int argc_err(int argc, char *argv[])
{
    if(argc < 4)
    {
        printf("Usage: %s filename server_ip_address server_port\n",argv[0]);
        printf("Example: %s /root/linux/server/test.txt 192.168.1.11 12345\n",argv[0]);
        return -1;
    }
    else
    {
        return 0;
    }
}


struct sockaddr_in* init_sockaddr_in(struct sockaddr_in *server_addr, char *ip, int port)
{
    bzero(&server_addr,sizeof (server_addr));
    server_addr->sin_family = AF_INET;
    inet_pton(AF_INET, ip, server_addr->sin_addr);
    server_addr->sin_port = htons((unsigned short)port);
}







/* Via ip address and port number by your given to connect server. */

int create_connect_socket(char *ip, int port)
{
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
        return -1;
    }
    else
    {
        return sock;
    }
}

#endif // CALL_INTERFACE_H
