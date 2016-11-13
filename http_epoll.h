/*
 * Copyright (C) Chenyang Li
 * Copyright (C) tinyhttpd
 */

#include <sys/epoll.h>

#define MAXEVENTS 1024

struct epoll_event *evlist;

int http_epoll_create(int flag);

int http_epoll_add(int epfd, int fd, struct epoll_event *event);

int http_epoll_mod(int epfd, int fd, struct epoll_event *event);

int http_epoll_del(int epfd, int fd, struct epoll_event *event);

int http_epoll_wait(int epfd, struct epoll_event *evlist, int maxevents, int timeout);
