/*
 * Copyright (C) Chenyang Li
 * Copyright (C) Vino
 * 
 * version 2017/12/01
 */
#ifndef VINO_VN_EPOLL_H
#define VINO_VN_EPOLL_H

#include <sys/epoll.h>

#define VN_EPOLL_FLAGS   0
#define VN_MAXEVENTS     1024

/*
typedef union epoll_data { 
    void *ptr; 
    int fd; 
    __uint32_t u32; 
    __uint64_t u64; 
} epoll_data_t;

struct epoll_event { 
    // Epoll events
    __uint32_t events;
    // User data variable
    epoll_data_t data; 
};
*/

/*
EPOLLIN ： 表示对应的文件描述符可读
EPOLLOUT： 表示对应的文件描述符可写
EPOLLPRI： 表示对应的文件描述符有紧急的数据可读
EPOLLERR： 表示对应的文件描述符发生错误 
EPOLLHUP： 表示对应的文件描述符被挂起 
EPOLLET ： 表示对应的文件描述符有事件发生
*/

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
 * referred by file descriptor `epollfd`.
 */
void vn_epoll_add(int epollfd, int fd, struct epoll_event *event);

/*
 * Change the event `event` associated with the target file descriptor `fd`.
 */
void vn_epoll_mod(int epollfd, int fd, struct epoll_event *event);

/*
 * Remove the target file descriptor `fd` from the epoll instance
 * referred by file descriptor `epollfd`.
 */
void vn_epoll_del(int epollfd, int fd, struct epoll_event *event);

/*
 * Wait for events on the epoll instance referred by the file
 * descriptor `epollfd`. The memory area pointed to by `events`
 * will contain the events that will be available for the caller.
 * Up to `maxevents` are returned.
 */
int vn_epoll_wait(int epollfd, struct epoll_event *events, int maxevents, int timeout);

#endif /* VINO_VN_EPOLL_H */
