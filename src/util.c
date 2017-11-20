/*
 *  Copyright (C) Chenyang Li
 *  Copyright (C) vino
 */
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "util.h"
#include "error.h"

int vn_get_string(vn_str *str, char *buf, size_t buf_len) {
    const char *s;
    unsigned int i;

    memset(buf, '\0', buf_len);
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

int vn_str_cmp(const vn_str *str1, const char *str2) {
    int i;
    const char *p1 = str1->p, *p2 = str2;
    size_t n1 = str1->len, n2 = strlen(str2);
    size_t shorter_len = n1 < n2 ? n1 : n2;
    
    for (i = 0; i < shorter_len && p1[i] == p2[i]; i++) {
        ;
    }
    
    if (i == shorter_len) {
        if (n1 == n2) { return 0; }
        if (shorter_len < n1) {
            return 1;
        } else {
            return -1;
        }
    } else {
        return (p1[i] < p2[i] ? -1 : 1); 
    }
   
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
    } else if (strstr(filepath, ".gif")) {
        strcpy(filetype, "image/gif");
    } else if (strstr(filepath, ".png")) {
        strcpy(filetype, "image/png");
    } else if (strstr(filepath, ".jpg") || strstr(filepath, ".jpeg")) {
        strcpy(filetype, "image/jpeg");
    } else {
        strcpy(filetype, "text/plain");
    }
}

unsigned int vn_get_filesize(const char *filepath) {
    struct stat sb;
    if (stat(filepath, &sb) != 0) {
        err_sys("[vn_get_filesize] stat error");
    }
    return sb.st_size;
}