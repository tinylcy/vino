/*
 * Copyright (C) Chenyang Li
 * Copyright (C) Vino
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "util.h"
#include "vn_error.h"

#define MIME_ENTRY(_ext, _type) { _ext, sizeof(_ext) - 1, _type }

static vn_mime_entry vn_builtin_mime_type[] = {
    MIME_ENTRY("html", "text/html"),
    MIME_ENTRY("htm", "text/html"),
    MIME_ENTRY("css", "text/css"),
    MIME_ENTRY("js", "text/javascript"),
    MIME_ENTRY("gif", "image/gif"),
    MIME_ENTRY("jpg", "image/jpeg"),
    MIME_ENTRY("jpeg", "image/jpeg"),
    MIME_ENTRY("png", "image/png"),
    MIME_ENTRY("svg", "image/svg+xml"),
    MIME_ENTRY("txt", "text/plain"),
    MIME_ENTRY("xml", "text/xml"),
    MIME_ENTRY("ttf", "application/x-font-ttf"),
    MIME_ENTRY("json", "application/json"),
    MIME_ENTRY("xslt", "application/xml"),
    MIME_ENTRY("xsl", "application/xml"),
    MIME_ENTRY("doc", "application/msword"),
    MIME_ENTRY("xls", "application/excel"),
    MIME_ENTRY("zip", "application/x-zip-compressed"),
    MIME_ENTRY("tgz", "application/x-tar-gz"),
    MIME_ENTRY("tar", "application/x-tar"),
    MIME_ENTRY("gz", "application/x-gunzip"),
    MIME_ENTRY("rar", "application/x-rar-compressed"),
    MIME_ENTRY("pdf", "application/pdf"),
    { NULL, 0, NULL }
};

int vn_get_string(vn_str_t *str, char *buf, size_t buf_len) {
    const char *s;
    unsigned int i;

    memset(buf, '\0', buf_len);

    if(!str->len) { return 0; }

    if (str->len + 1 > buf_len) { return -1; }

    s = str->p;
    i = 0;
    while (i < str->len) {
        buf[i] = s[i];
        i++;
    }
    buf[i] = '\0';
    return 0;
}

int vn_str_cmp(const vn_str_t *str1, const char *str2) {
    int i;
    const char *p1 = str1->p, *p2 = str2;
    size_t n1 = str1->len, n2 = strlen(str2);
    size_t shorter_len = n1 < n2 ? n1 : n2;
    
    for (i = 0; i < shorter_len && p1[i] == p2[i]; i++) { ; }
    
    if (i == shorter_len) {
        if (n1 == n2) { return 0; }
        if (shorter_len < n1) {
            return 1;
        } else {
            return -1;
        }
    } else {
        return (p1[i] < p2[i] ? -1 : 1); 
    }
   
}

int vn_str_atoi(const vn_str_t *str) {
    char *buf;
    int rv;

    if ((buf = (char *) malloc(sizeof(char) * (str->len + 1))) == NULL) {
        err_sys("[vn_str_atoi] malloc error");
    }
    strncpy(buf, str->p, str->len);
    buf[str->len] = '\0';
    rv = atoi(buf);
    return rv;
}

void vn_check_null(int num, ...) {
    int i;
    va_list ap;
    va_start(ap, num);
    for (i = 0; i < num; i++) {
        if (va_arg(ap, void *) == NULL) {
            err_sys("[vn_check_null] NULL pointer error");
        }
    }
    va_end(ap);
}

const char *vn_skip(const char *s, const char *end, const char *delims,
                         vn_str_t *vs) {
    vs->p = s;
    while (s < end && strchr(delims, *s) == NULL) { s++; }
    vs->len = s - vs->p;
    while (s < end && strchr(delims, *s) != NULL) { s++; } 
    return s;                    
}

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

int vn_check_file_exist(const char *filepath) {
    if (0 == access(filepath, F_OK)) {
        return 0;
    } else {
        return -1;
    }
}

int vn_check_read_permission(const char *filepath) {
    struct stat sb;
    pid_t prog_pid;
    uid_t prog_uid;
    gid_t prog_gid;
    char proc_path[20];

    /* Get UID and PID of the process */
    prog_pid = getpid();
    sprintf(proc_path, "/proc/%d", prog_pid);
    if (stat(proc_path, &sb) < 0) {
        err_sys("[vn_check_read_permission] stat proc_path error");
    }
    prog_uid = sb.st_uid;
    prog_gid = sb.st_gid;

    /* Get UID and PID of the file */
    if (stat(filepath, &sb) < 0) {
        err_sys("[vn_check_read_permission] stat filepath error");
    }

    /* If the process own the file, check if it has read access. */
    if (prog_uid == sb.st_uid && (sb.st_mode & S_IRUSR)) {
        return 0;
    }
    
    /* 
     * Check if the group of the process's UID matches the file's group,
     * if so, check for read access.
     */
    if (prog_gid == sb.st_gid && (sb.st_mode & S_IRGRP)) {
        return 0;
    } 
    
    /*
     * The process's UID is neither the owner of the file nor does its GID
     * match the file's. Check whether the file is world readable. 
     */ 
    if (sb.st_mode & S_IROTH) {
        return 0;
    }

    return -1;
}

void vn_get_mime_type(const char *filepath, char *mime_type) {
    vn_mime_entry entry;
    size_t i, path_len;
    char *dot;

    path_len = strlen(filepath);
    if ((dot = strrchr(filepath, '.')) == NULL || filepath[path_len - 1] == '.') {
        strcpy(mime_type, "text/plain");
        return;
    }
    for (i = 0; vn_builtin_mime_type[i].extension != NULL; i++) {
        entry = vn_builtin_mime_type[i];
        if (strlen(dot + 1) == entry.ext_len && strncmp(dot + 1, entry.extension, entry.ext_len) == 0) {
            strcpy(mime_type, entry.mime_type);
            return;
        }
    }

    strcpy(mime_type, "text/plain");
}

unsigned int vn_get_filesize(const char *filepath) {
    struct stat sb;
    if (stat(filepath, &sb) < 0) {
        err_sys("[vn_get_filesize] stat error");
    }
    return sb.st_size;
}

void vn_signal(int signum, void (*handler)(int)) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(struct sigaction));
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);

    if (signum == SIGALRM) {
#ifdef SA_INTERRUPT
        sa.sa_flags |= SA_INTERRUPT;
#endif
    } else {
        sa.sa_flags |= SA_RESTART;
    }
    
    if (sigaction(signum, &sa, NULL) < 0) {
        err_sys("[vn_signal] sigaction error");
    }
}

void vn_parse_config(const char *conf_file, vn_conf *conf) {
    FILE *fp;
    char line[VN_MAXLINE];
    char *key_start, *key_end, *value_start, *value_end;

    if (vn_check_file_exist(conf_file) < 0) {
        err_sys("[vn_parse_config] vn_check_file_exist error: config file can not be found.");
    }
    // TODO: Check read permission.
    if ((fp = fopen(conf_file, "r")) == NULL) {
        err_sys("[vn_parse_config] fopen error");
    }

    while (fgets(line, VN_MAXLINE, fp) != NULL) {
        if (strlen(line) == 0 || line[0] == '#' || strchr(line, '=') == NULL) {
            continue;
        }

        key_start = line;
        while (NULL != key_start && isspace(*key_start)) { key_start++; }
        key_end = strchr(key_start, '=');
        vn_check_null(2, key_start, key_end);
        value_start = key_end;
        while (NULL != value_start && (isspace(*value_start) || *value_start == '=')) { value_start++; }
        value_end = value_start;
        while (NULL != value_end && !isspace(*value_end) && *value_end != CR && *value_end != LF) { value_end++; }
        vn_check_null(4, key_start, key_end, value_start, value_end);

        if (!strncasecmp(key_start, "port", key_end - key_start)) {
            strncpy(conf->port, value_start, value_end - value_start);
        }

        if (!strncasecmp(key_start, "ip", key_end - key_start)) {
            strncpy(conf->ip, value_start, value_end - value_start);
        }

    }

    if (fclose(fp) < 0) {
        err_sys("[vn_parse_config] flose error");
    }
}