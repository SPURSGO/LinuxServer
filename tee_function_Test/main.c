#define _GNU_SOURCE  /* 使用splice函数不仅要指定头文件fcntl.h,还要指定此宏 */
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        printf("Usage: %s <file>\n",argv[0]);
        return -1;
    }

    int filefd = open(argv[1], O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if(filefd < 0)
    {
        printf("open file fail! And errno is %d\n",errno);
        return -1;
    }

    int pipefd_stdout[2];
    int ret = pipe(pipefd_stdout);
    assert(ret != -1);

    int pipefd_file[2];
    ret = pipe(pipefd_file);
    assert(ret != -1);

    /* 将标准输入的内容输入管道 */
    ret = (int)splice(STDIN_FILENO, NULL, pipefd_stdout[1], NULL, 32786, SPLICE_F_MORE | SPLICE_F_MOVE);
    assert( ret != -1);

    /* 将管道pipefd_stdout的输出复制到管道pipefd_file的输入端 */
    ret = (int)tee(pipefd_stdout[0], pipefd_file[1], 32678, SPLICE_F_NONBLOCK);
    assert( ret != -1);

    /* 将管道pipefd_file的输出定向到文件描述符filefd上，从而将标准输入的内容写入文件 */
    ret = (int)splice(pipefd_file[0], NULL, filefd, NULL, 32678, SPLICE_F_MOVE | SPLICE_F_MOVE);
    assert( ret != -1);

    /* 将管道pipefd_stdout中的输出定向到标准输出，其内容和写入文件的内容完全一致 */
    ret = (int)splice(pipefd_stdout[0], NULL, STDOUT_FILENO, NULL, 32678, SPLICE_F_MORE | SPLICE_F_MOVE);
    assert( ret != -1);

    close(filefd);
    close(pipefd_file[0]);
    close(pipefd_file[1]);
    close(pipefd_stdout[0]);
    close(pipefd_stdout[1]);

    return 0;
}
