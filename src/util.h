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
 * Fetch specified-length string from `str` into `buf`. 
 * 
 *  0 - Fetch success
 * <0 - Fetch failed 
 */
int vn_get_string(vn_str *str, char *buf, size_t buf_len);

/* 
 * Set the O_NONBLOCK flag on the descriptor.
 */
int make_socket_non_blocking(int sockfd);


/*
 * Compare a self-defined string with a regular string.
 */
int vn_str_cmp(const vn_str *str1, const char *str2);

/*
 * Check if a file exist.
 * 0  - Exist
 */ 
int vn_check_file_exist(const char *filepath);

/*
 * Derive file type from file name.
 */
void vn_get_filetype(const char *filepath, char *filetype);

/*
 * Return the size of the file.
 */
unsigned int vn_get_filesize(const char *filepath);

#endif