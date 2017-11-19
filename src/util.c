/*
 *  Copyright (C) Chenyang Li
 *  Copyright (C) vino
 */
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "util.h"
#include "error.h"

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