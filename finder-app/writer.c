#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>

int main(int argc, char *argv[]) {
    openlog(NULL, LOG_PID|LOG_CONS, LOG_USER);

    // 检查参数数量是否小于2
    if (argc < 3) {
        syslog(LOG_ERR, "Please specify both the file dir and string to write, only get %d and %s", argc - 1, argv[1] ? argv[1] : "NULL");
        closelog();
        exit(EXIT_FAILURE);
    }

    // 获取文件路径和要写入的字符串
    char *fileDir = argv[1];
    char *writeStr = argv[2];
    char *dir = strdup(fileDir); // 复制文件路径以避免修改原始参数

    // 获取目录路径
    char *baseDir = dirname(dir);

    // 检查目录是否存在
    struct stat st;
    if (stat(baseDir, &st) == 0 && S_ISDIR(st.st_mode)) {
        // 目录存在，写入文件
        FILE *file = fopen(fileDir, "w");
        if (file == NULL) {
            syslog(LOG_ERR, "Error opening file: %s", fileDir);
            free(dir);
            closelog();
            exit(EXIT_FAILURE);
        }
        fprintf(file, "%s", writeStr);
        fclose(file);
    } else {
        // 目录不存在，创建目录并写入文件
        if (mkdir(baseDir, 0777) != 0 && errno != EEXIST) {
            syslog(LOG_ERR, "Error creating directory: %s", baseDir);
            free(dir);
            closelog();
            exit(EXIT_FAILURE);
        }
        FILE *file = fopen(fileDir, "w");
        if (file == NULL) {
            syslog(LOG_ERR, "Error opening file: %s", fileDir);
            free(dir);
            closelog();
            exit(EXIT_FAILURE);
        }
        fprintf(file, "%s", writeStr);
		syslog(LOG_DEBUG, "Writing %s to %s", writeStr, fileDir);
        fclose(file);
    }

    // 释放分配的内存
    free(dir);

    syslog(LOG_DEBUG, "File written successfully");
    closelog();
    return EXIT_SUCCESS;
}

