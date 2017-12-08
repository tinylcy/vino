/*
 * Copyright (C) Chenyang Li
 * Copyright (C) Vino
 */
#ifndef VINO_VN_EPOLL_H
#define VINO_VN_EPOLL_H

#include <sys/epoll.h>

#define VN_EPOLL_FLAGS   0
#define VN_MAXEVENTS     1024

// TODO: free events memory
/* The events that will be available for caller */
struct epoll_event *events;

/* 
 * Initialize epoll event list 
 */
void vn_epoll_init(void);

/* 
 * Open an epoll file descriptor 
 */
int vn_epoll_create(void);

/* 
 * Register the target file descriptor `fd` on the epoll instance 
 * referred by file descriptor `epfd` and associate the ADD event 
 * `event` with the internal file linked to `fd`.
 */
void vn_epoll_add(int epfd, int fd, struct epoll_event *event);

/*
 * Change the event `event` associated with the target file
 * descriptor `fd`.
 */
void vn_epoll_mod(int epfd, int fd, struct epoll_event *event);

/*
 * Remove the target file descriptor `fd` from the epoll instance
 * referred by `epfd`. The `event` is ignored and can be NULL. 
 */
void vn_epoll_del(int epfd, int fd, struct epoll_event *event);

/*
 * Wait for events on the epoll instance referred by the file
 * descriptor `epfd`. The memory area pointed to by `events`
 * will contain the events that will be available for the caller.
 * Up to `maxevents` are returned.
 */
int vn_epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);

#endif /* VINO_VN_EPOLL_H */
