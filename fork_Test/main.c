#include <stdio.h>
#include <unistd.h>

int main()
{
//    pid_t pid;
//    int count = 0;
//    pid = fork();
//    if( pid < 0)
//    {
//        printf("error in fork!");
//    }
//    else if( pid == 0 )
//    {
//        printf(" i am the child process. my pid is %d and my parent pid is %d\n", getpid(), getppid() );
//        count++;
//    }
//    else
//    {
//        printf("i am the parent process. my pid is %d and my parent pid is %d\n", getpid(), getppid() );
//        count++;
//    }
//    printf("the count = %d\n", count);

    int i = 0;
    printf("i child/parent ppid pid fork_pid\n");
    for(i = 0; i<2; ++i)
    {
        pid_t fork_pid = fork();
        if(fork_pid == 0)
        {
             printf("%d  child   %4d   %4d  %4d\n",i,getppid(),getpid(),fork_pid);
        }
        else
        {
            printf("%d  parent   %4d   %4d  %4d\n",i,getppid(),getpid(),fork_pid);

        }
    }
    return 0;
}







