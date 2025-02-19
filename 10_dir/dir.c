#include <stdio.h>  
#include <stdlib.h>  
#include <stddef.h>
#include <dirent.h>  
#include <sys/types.h>
#include <string.h>  
  
int count_files_in_dir(const char *dir_path, int *file_count) {  
    DIR *dir;  
    struct dirent *entry;  
    char path[1024];  //缓冲字符串
  
    if (!(dir = opendir(dir_path))) {  
        perror("opendir");  
        return 1;  
    }  
  
    while (entry = readdir(dir)) {  
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)  
            continue;  
  
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);  //拼接字符串, 形成目录树

        if (entry->d_type == DT_REG) {  
            (*file_count)++;  
        } else if (entry->d_type == DT_DIR) {              
            count_files_in_dir(path, file_count);  
        }  
    }  
  
    closedir(dir);  
    return 0;  
}  
  
int main(int argc, char *argv[]) {  
    if (argc != 2) {  
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);  
        return 1;  
    }  
  
    int file_count = 0;  
    if (count_files_in_dir(argv[1], &file_count) == 0) {  
        printf("Number of regular files: %d\n", file_count);  
    }  
  
    return 0;  
}