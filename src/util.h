/*
 *  Copyright (C) Chenyang Li
 *  Copyright (C) vino
 */
#ifndef VINO_UTIL_H
#define VINO_UTIL_H

/* Self-defined string structure */
typedef struct vn_str_s {
    const char *p;         /* String pointer */
    size_t     len;        /* String length */
} vn_str;

/* 
 * Set the O_NONBLOCK flag on the descriptor.
 */
int make_socket_non_blocking(int sockfd);


/*
 * Compare a self-defined string with a regular string.
 */
int vn_str_cmp(const vn_str *str1, const char *str2);

#endif