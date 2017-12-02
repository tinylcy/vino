/*
 * Copyright (C) Chenyang Li
 * Copyright (C) Vino
 */
#include <stdio.h>
#include <stdlib.h>
#include "vn_epoll.h"
#include "vn_error.h"

void vn_epoll_init(void) {
    events = (struct epoll_event *) malloc(sizeof(struct epoll_event) * VN_MAXEVENTS);
    if (NULL == events) {
        err_sys("[vn_epoll_init] malloc error");
    }
}

int vn_epoll_create(void) {
    int epfd;
    if ((epfd = epoll_create1(VN_EPOLL_FLAGS)) == -1) {
        err_sys("[vn_epoll_create] epoll_create1 error");
    }
    return epfd;
}

void vn_epoll_add(int epfd, int fd, struct epoll_event *event) {
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, event) < 0) {
        err_sys("[vn_epoll_add] epoll_ctl error");
    }
}

void vn_epoll_mod(int epfd, int fd, struct epoll_event *event) {
    if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, event) < 0) {
        err_sys("[vn_epoll_mod] epoll_ctl error");
    }
}

void vn_epoll_del(int epfd, int fd, struct epoll_event *event) {
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, event) < 0) {
        err_sys("[vn_epoll_del] epoll_ctl error");
    }
}

int vn_epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout) {
    int nready;
    if ((nready = epoll_wait(epfd, events, maxevents, timeout)) < 0) {
        err_sys("[vn_epoll_wait] epoll_wait error");
    }
    return nready;
}