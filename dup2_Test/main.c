#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

/* dup/dup2 test case. */
int main()
{

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);  //sockfd = 3
    if(sockfd < 0)
    {
        printf("call to socket fial! And error number is %d\n",errno);
        return -1;
    }

    /*  #include <unistd.h>
     *  int dup(int exist_fd);
     *  dup用来复制参数exist_fd文件描述符。
     *  当复制成功时，返回最小的尚未被使用过的文件描述符，出错时，返回-1并设置errno.
     *  返回的新文件描述符和参数exist_fd指向同一个文件.
     *
     *  --------------------------------------------------------------------
     *
     *  #include <unistd.h>
     *  int dup2(int exist_fd, int specified_fd);
     *  dup2与dup区别是dup2可以用参数specified_fd指定新文件描述符的值。
     *  若参数specified_fd文件描述符已经被本进程使用，则本进程就会将specified_fd所指的文件关闭
     *  若specified_fd等于exist_fd，则返回specified_fd,而不关闭exist_fd所指的文件。
     *  若dup2调用成功则返回新的文件描述符，出错则返回-1并设置errno.
     *
     *  注意：通过dup和dup2创建的文件描述符并不继承原文件描述符的属性，比如close-on-exec和non-blocking等
     */

    int dup_ret1 = dup(sockfd); // dup_ret1 = 4
    int dup2_ret = dup2(sockfd,5); // fd_arg = 5 ---> fd_arg = 100 ---> fd_arg = 4 --->fd_arg = 3
    int dup_ret2 = dup(sockfd);

    int empty_dup = dup(101);/* 指定复制一个不存在的文件描述符，则将会出错返回-1并设置errno=9 */
                             /* #define	EBADF		 9	 Bad file number  */
    if(empty_dup < 0)
    {
        printf("dup fail! errno is %d\n", errno);
    }

    printf("sockfd = %d, dup_ret1 = %d, dup2_ret = %d, dup_ret2 = %d\n",sockfd, dup_ret1, dup2_ret, dup_ret2);
    return 0;
}
