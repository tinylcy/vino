/*
 * Copyright (C) Chenyang Li
 * Copyright (C) Vino
 */
#ifndef VINO_VN_PALLOC_H
#define VINO_VN_PALLOC_H

#include <stdio.h>

#define VN_DEFAULT_POOL_SIZE     (16 * 1024)
#define VN_POOL_ALIGNMENT        16
#define VN_MAX_ALLOC_FROM_POOL   (4 * 1024 - 1)

typedef struct vn_pool_s vn_pool_t;
typedef struct vn_pool_large_s vn_pool_large_t;

typedef struct vn_pool_data_s {
    char           *last;
    char           *end;
    vn_pool_t      *next;
    unsigned int    failed;
} vn_pool_data_t;

typedef struct vn_pool_large_s {
    vn_pool_large_t   *next;
    void              *alloc;
} vn_pool_large_t;

struct vn_pool_s {
    vn_pool_data_t     data;
    size_t             max;
    vn_pool_t         *current;
    vn_pool_large_t   *large;
};

vn_pool_t *vn_create_pool(size_t size);
void vn_destroy_pool(vn_pool_t *pool);
void vn_reset_pool(vn_pool_t *pool);

void *vn_palloc(vn_pool_t *pool, size_t size);
void *vn_pcalloc(vn_pool_t *pool, size_t size);
void *vn_pmemalign(vn_pool_t *pool, size_t size, size_t alignment);
int vn_pfree(vn_pool_t *pool_t, void *p);
// void vn_print_pool(vn_pool_t *pool);

#endif /* VINO_VN_PALLOC_H */