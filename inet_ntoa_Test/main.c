#include <stdio.h>
#include <arpa/inet.h>


/*
 * 此程序用来验证函数inet_ntoa[ char* inet_ntoa(struct in_addr in); ]的不可重入性。
 * 因为该函数内部用一个静态变量存储转化结果，函数的返回值指向该静态内存。
 *
 */

int main()
{
    //调用inet_aton函数可以将字符串ip地址转换成网络字节序整数表示的ip地址
    struct in_addr addr1 ;
    struct in_addr addr2 ;

    //可以用以下方法初始化in_addr结构体
    if(!inet_aton("1.2.3.4",&addr1))   //在给inet_aton()传递第二个参数时需要特别注意！！！
        printf("failed to call inet_aton.");
    if(!inet_aton("10.20.30.40",&addr2))
        printf("failed to call inet_aton.");


    //将网络字节序整数表示的IPV4地址转化为用点分十进制字符串表示的IPV4地址
    char* str_inet_addr1 = inet_ntoa(addr1);
    printf("address 1:%s\n",str_inet_addr1); //显示在第二次调用inet_ntoa之前，其静态内存中的内容

    char* str_inet_addr2 = inet_ntoa(addr2);
    printf("address 1:%s\n",str_inet_addr1);
    printf("address 2:%s\n",str_inet_addr2);

    return 0;
}
