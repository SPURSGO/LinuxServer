#include <stdio.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

union semun
{
    int val;               /* 用于SETVAL命令 */
    struct semid_ds* buf;  /* semid_ds是与信号量集相关联的内核数据结构,此变量用于IPC_STAT,IPC_SET命令 */
    unsigned short* array; /* 用于GETALL、SETALL命令 */
    struct seminfo* _buf;  /* 用于IPC_INFO命令 struct seminfo--->bits/sem.h*/
};


void pv_op(int sem_id, int op)
{
    struct sembuf sem_b;
    sem_b.sem_num = 0;         //指定要操作的信号量是信号量集中的第一个信号量
    sem_b.sem_op = (short)op;  // 指定操作的类型，op为-1的时候执行P操作，为1时执行V操作.
    sem_b.sem_flg = SEM_UNDO;  // 当程序退出时取消正在进行的semop操作,即使sem_ops数组中还有其他信号量操作未执行
    semop(sem_id, &sem_b, 1);  //在此，只对信号量集中的第一个信号量执行p、v操作
}


int main()
{
    int sem_id = semget(IPC_PRIVATE, 1, 0666);  //0666指定一组标志，低端9bit指定调用进程对该信号量集的权限

    union semun sem_un;
    sem_un.val = 1;  // 以此值来设置某个信号量的值(内核变量:semval)
    semctl(sem_id, 0, SETVAL, sem_un);  // semctl是一个参数个数可变函数(可变参数)

    pid_t pid = fork();  // fork() --->unistd.h
    if(pid < 0)
    {
        return 1;
    }
    else if(pid == 0)
    {
        printf("child try to get binary semaphore.\n");
        /* 在父子进程间共享IPC_PRIVATE信号量的关键就在于二者都可以操作该信号量的标识符sem_id */
        pv_op(sem_id, -1);  // P
        printf("child get the semaphore and would release it after 5 seconds.\n");  //临界区
        sleep(1);
        pv_op(sem_id, 1);
        exit(0);
    }
    else
    {
        printf("parent try to get binary semaphore.\n");
        pv_op(sem_id, -1);
        printf("parent get the semaphore and would release it after 5 seconds\n");
        sleep(1);
        pv_op(sem_id, 1);
    }

    waitpid(pid, NULL, 0);
    semctl(sem_id, 0, IPC_RMID, sem_un);

    return 0;
}




