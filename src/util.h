/*
 * Copyright (C) Chenyang Li
 * Copyright (C) Vino
 */
#ifndef VINO_UTIL_H
#define VINO_UTIL_H

#ifndef VN_MAXLINE
#define VN_MAXLINE 1024
#endif

#define VN_PORTBUF 6
#define VN_IPBUF   15
#define CR  '\r'
#define LF  '\n'

/* Self-defined string structure */
typedef struct vn_str_s {
    const char    *p;         /* String pointer */
    size_t        len;        /* String length */
} vn_str;

/*  Vino configuration infomation */
typedef struct vn_conf_s {
    char port[VN_PORTBUF];
    char ip[VN_IPBUF];
} vn_conf;

/*
 * Fetch specified-length string from `str` into `buf`. 
 * 
 *  0 - Fetch success
 * <0 - Fetch failed 
 */
int vn_get_string(vn_str *str, char *buf, size_t buf_len);

/*
 * Check NULL pointer parameters. 
 * `num` is the number of parapeters.
 */
void vn_check_null(int num, ...);

/*
 * Compare a self-defined string with a regular string.
 */
int vn_str_cmp(const vn_str *str1, const char *str2);

/*
 * Convert a self-defined string pointed by `str` to int.
 */ 
int vn_str_atoi(const vn_str *str);

/*
 * Fetch substring from `s` to `end` into `vs`.
 * 
 * Skip initial delimiters characters. Record the first non-delimiter
 * character at the beginning of `vs`. Then scan the rest of the string 
 * until a delimiter character or end-of-string is found.
 */
const char *vn_skip(const char *s, const char *end,
                        const char *delims, vn_str *vs);

/* 
 * Set the O_NONBLOCK flag on the descriptor.
 */
int make_socket_non_blocking(int sockfd);

/*
 * Check if a file exist.
 * 0  - Exist
 */ 
int vn_check_file_exist(const char *filepath);

/*
 * Check if the process has permission to read file.
 *  0 - Yes
 * <0 - No
 */ 
int vn_check_read_permission(const char *filepath);

/*
 * Derive file type from file name.
 */
void vn_get_filetype(const char *filepath, char *filetype);

/*
 * Return the size of the file.
 */
unsigned int vn_get_filesize(const char *filepath);

/*
 * Set the disposition of the signal `signum` to `handler`.
 */
void vn_signal(int signum, void (*handler)(int));

/*
 * Parse the configuration file `conf_file` and store the information into `conf`.
 */ 
void vn_parse_config(const char *conf_file, vn_conf *conf);

#endif /* VINO_UTIL_H */