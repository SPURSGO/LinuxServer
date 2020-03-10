#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    int fd = open("hello.c", O_CREAT | O_RDONLY | O_TRUNC, 0611);
    if( fd < 0 )
    {
        printf("open hello.c error!\n");
        exit(1);
    }
    else
    {
        printf("open hello.c successfully and the fd is %d\n",fd);
    }

    if(close(fd) < 0)
    {
        perror("close file error!\n");
    }
    else
    {
        printf("close file successfully!\n");
    }

    return 0;
}
