/*
 * Copyright (C) Computer Systems: A Programmer's Perspective
 * Copyright (C) CSAPP
 */
#ifndef VINO_RIO_H
#define VINO_RIO_H

#include <sys/types.h>

#define RIO_BUFSIZE 8192

typedef struct rio {
    int  rio_fd;                    /* Subsriptor for this internal buf */
    int  rio_cnt;                   /* Unread bytes in internal buf */
    char *rio_bufptr;               /* Next unread byte in internal buf */
    char rio_buf[RIO_BUFSIZE];      /* Internal buffer */
} rio_t;

ssize_t rio_readn(int fd, void *userbuf, size_t n);

ssize_t rio_writen(int fd, void *userbuf, size_t n);

void rio_readinitb(rio_t *rp, int fd);

ssize_t rio_readlineb(rio_t *rp, void *userbufm, size_t maxlen);

ssize_t rio_readnb(rio_t *rp, void *userbuf, size_t n);

#endif /* VINO_RIO_H */