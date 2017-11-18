/*
 *  Copyright (C) Chenyang Li
 *  Copyright (C) vino
 */
#include <unistd.h>
#include <fcntl.h>

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
