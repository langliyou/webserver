#include <stdio.h>
#include <arpa/inet.h>

int main()
{
    union 
    {
        short a;
        char data[sizeof(short)];
    } test;
    test.a = 0x0102;
    if(test.data[0] == 1)
    {
        printf("big endian.\n");
    }
    else if(test.data[0] == 2)
    {
        printf("little endian.\n");

    }
    return 0;
}