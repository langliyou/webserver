#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
// #define ARGV

int main(int argc, char *argv[])
{
#ifdef ARGV
    for (int i = 0; i < argc; ++i)
    {
        printf("argv[%d] = %s \n", i, argv[i]);
    }
#endif
    struct stat sb;
    if (stat(argv[1], &sb) == -1)
    {
        perror("stat");
        return -1;
    }
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <pathname>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    char perms[11] = "----------";
    switch (sb.st_mode & S_IFMT)
    {
    case S_IFBLK:
        perms[0] = 'b';
        break;
    case S_IFCHR:
        perms[0] = 'c';
        break;
    case S_IFDIR:
        perms[0] = 'd';
        break;
    case S_IFIFO:
        perms[0] = 'p';
        break;
    case S_IFLNK:
        perms[0] = 'l';
        break;
    case S_IFREG:
        perms[0] = '-';
        break;
    case S_IFSOCK:
        perms[0] = 's';
        break;
    default:
        perms[0] = '?';
        break;
    }
    perms[1] = sb.st_mode & S_IRUSR? 'r': '-';
    perms[2] = sb.st_mode & S_IWUSR? 'w': '-';
    perms[3] = sb.st_mode & S_IXUSR? 'x': '-';
    perms[4] = sb.st_mode & S_IRGRP? 'r': '-';
    perms[5] = sb.st_mode & S_IWGRP? 'w': '-';
    perms[6] = sb.st_mode & S_IXGRP? 'x': '-';
    perms[7] = sb.st_mode & S_IROTH? 'r': '-';
    perms[8] = sb.st_mode & S_IWOTH? 'w': '-';
    perms[9] = sb.st_mode & S_IXOTH? 'x': '-';
    
    // struct group *gr = getgrgid(sb.st_gid);
    char buf[1024];
    char *time = ctime(&sb.st_mtime);
    int len = strlen(time);
    if(len > 1) time[len - 1] = '\0';//逻辑删除最后一个字符
    sprintf(buf, "%s %ju %s %s %ju %s %s", 
        perms, (uintmax_t)sb.st_nlink, getpwuid(sb.st_uid)->pw_name, getgrgid(sb.st_gid)->gr_name, (uintmax_t)sb.st_ino, time, argv[1]);
    printf("%s\n", buf);
    return 0;
}