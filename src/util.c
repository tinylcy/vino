/*
 * Copyright (C) Chenyang Li
 * Copyright (C) Vino
 *
 * version 2017/12/01
 */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "util.h"
#include "error.h"

int vn_get_string(vn_str *str, char *buf, size_t buf_len) {
    const char *s;
    size_t i;

    // 清空buf内存区域
    memset(buf, '\0', buf_len);
    // 缓存容量不够则返回
    if (str->len + 1 > buf_len) {
        return -1;
    }

    s = str->p;
    i = 0;
    while (i < str->len) {
        buf[i] = s[i];
        i++;
    }
    buf[i] = '\0';

    return 0;
}

int vn_str_cmp(const vn_str *str1, const char *str2) {
    size_t i;
    const char *p1 = str1->p, *p2 = str2;
    size_t n1 = str1->len, n2 = strlen(str2);
    size_t shorter_len = n1 < n2 ? n1 : n2;
    
    for (i = 0; i < shorter_len && p1[i] == p2[i]; i++) { ; }
    
    if (i == shorter_len) {
        if (n1 == n2) { 
            return 0; 
        }
        else if (shorter_len < n1) {
            return 1;
        } else {
            return -1;
        }
    } else {
        return (p1[i] < p2[i] ? -1 : 1); 
    }
}

void vn_check_null(int num, ...) {
    size_t i;
    va_list ap;
    va_start(ap, num);
    for (i = 0; i < num; i++) {
        if (va_arg(ap, void*) == NULL) {
            err_sys("[vn_check_null] NULL pointer error");
        }
    }
    va_end(ap);
}

int make_socket_non_blocking(int sockfd) {
    int flags;

    if ((flags = fcntl(sockfd, F_GETFL, 0)) == -1) {
        err_sys("[make_socket_non_blocking] fcntl F_GETFL error");
    }
    flags |= O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) == -1) {
        err_sys("[make_socket_non_blocking] fcntl F_SETFL error");
    }

    return 0;
}

int vn_check_file_exist(const char *filepath) {
    if (0 == access(filepath, F_OK)) {
        return 0;
    } else {
        return -1;
    }
}

void vn_get_filetype(const char *filepath, char *filetype) {
    if (strstr(filepath, ".html")) {
        strcpy(filetype, "text/html");
    }
    else if (strstr(filepath, ".css")) {
        strcpy(filetype, "text/css");
    }
    else if (strstr(filepath, ".asp")) {
        strcpy(filetype, "text/asp");
    }
    else if (strstr(filepath, ".java")) {
        strcpy(filetype, "java/*");
    }
    else if (strstr(filepath, ".gif")) {
        strcpy(filetype, "image/gif");
    }
    else if (strstr(filepath, ".png")) {
        strcpy(filetype, "image/png");
    }
    else if (strstr(filepath, ".jpg") || strstr(filepath, ".jpeg")) {
        strcpy(filetype, "image/jpeg");
    }
    else if (strstr(filepath, ".tiff")) {
        strcpy(filetype, "image/tiff");
    }
    else if (strstr(filepath, ".xml")) {
        strcpy(filetype, "text/xml");
    }
    else {
        strcpy(filetype, "text/plain");
    }
}

size_t vn_get_filesize(const char *filepath) {
    // 获取文件属性
    struct stat sbuf;
    if (stat(filepath, &sbuf) != 0) {
        err_sys("[vn_get_filesize] stat error");
    }
    return sbuf.st_size;
}

void vn_signal(int signum, void (*handler)(int)) {
    // 查询或设置信号处理方式
    struct sigaction sa;
    memset(&sa, '\0', sizeof(struct sigaction));
    sa.sa_handler = handler;
    sa.sa_flags = 0;
    if (sigaction(signum, &sa, NULL) < 0) {
        err_sys("[vn_signal] sigaction error");
    }
}

void vn_parse_config(const char *conf_file, vn_conf *conf) {
    FILE *fp;
    char line[VN_MAXLINE];
    char *key_start, *key_end, *value_start, *value_end;

    if (vn_check_file_exist(conf_file) < 0) {
        err_sys("[vn_parse_config] vn_check_file_exist error: config file can not be found.");
    }
    struct stat sbuf;
    if (stat(conf_file, &sbuf) != 0) {
        err_sys("[vn_parse_config] stat error");
    }
    // 检查用户是否具有文件读权限
    if ((sbuf.st_mode & S_IRWXU) != S_IRUSR) {
        err_sys("[vn_parse_config] read permission denied");
    }
    if ((fp = fopen(conf_file, "r")) == NULL) {
        err_sys("[vn_parse_config] fopen error");
    }

    while (fgets(line, VN_MAXLINE, fp) != NULL) {
        if (strlen(line) == 0 || line[0] == '#' || strchr(line, '=') == NULL) {
            continue;
        }

        key_start = line;
        while (NULL != key_start && isspace(*key_start)) { key_start++; }
        key_end = strchr(key_start, '=');
        vn_check_null(2, key_start, key_end);
        value_start = key_end;
        while (NULL != value_start && (isspace(*value_start) || *value_start == '=')) { value_start++; }
        value_end = value_start;
        while (NULL != value_end && !isspace(*value_end) && (*value_end != CR && *value_end != LF)) { value_end++; }
        vn_check_null(4, key_start, key_end, value_start, value_end);

        if (!strncasecmp(key_start, "ip", key_end - key_start)) {
            strncpy(conf->ip, value_start, value_end - value_start);
        }

        if (!strncasecmp(key_start, "port", key_end - key_start)) {
            strncpy(conf->port, value_start, value_end - value_start);
        }
    }

    if (fclose(fp) < 0) {
        err_sys("[vn_parse_config] flose error");
    }
}
