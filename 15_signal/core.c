#include <string.h>
#include <stdio.h>

int main()
{
    char *p = NULL;
    strcpy(p, "hello worldaaaaaaaaaa");
    //不产生core文件？？？
    return 0;
}