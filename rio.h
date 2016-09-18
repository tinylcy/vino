#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define RIO_BUFSIZE 8192

typedef struct {
	int rio_fd;    // descriptior for this internal buffer
	int rio_cnt;    // unread bytes in internal buffer
	char *rio_bufptr;    // next unread byte in internal buffer
	char rio_buf[RIO_BUFSIZE];    // internal buffer
} rio_t;

ssize_t rio_readn(int fd, void *buf, size_t n);
ssize_t rio_write(int fd, void *buf, size_t n);

void rio_readinitb(rio_t *rp, int fd);

ssize_t rio_readlineb(rio_t *rp, void *buf, size_t maxlen);
ssize_t rio_readnb(rio_t *rp, void *buf, size_t n);
