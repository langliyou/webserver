#include <stdio.h>
#include <arpa/inet.h>

int main()
{
    const char ip[] = "192.168.1.5";
    unsigned char buf[16];
    inet_pton(AF_INET, ip, buf);
    printf("pton: %d.%d.%d.%d\n", *buf, *(buf + 1), *(buf + 2), *(buf + 3));
    unsigned char buf2[16];
    inet_ntop(AF_INET, buf, buf2, sizeof(buf));
    printf("ntop: %s\n", buf2);
    return 0;
}