/*
 * Copyright (C) Chenyang Li
 * Copyright (C) Vino
 */
#include <stdlib.h>
#include <string.h>
#include "vn_alloc.h"
#include "vn_error.h"

void *vn_alloc(size_t size) {
    void *p;

    p = malloc(size);
    if (NULL == p) {
        err_sys("vn_alloc malloc error");
    }

    return p;
}

void *vn_calloc(size_t size) {
    void *p;

    p = vn_alloc(size);
    if (p) {
        memset(p, '\0', size);
    }
    return p;
}

void vn_free(void *p) {
    free(p);
}