/*
 * Copyright (C) Chenyang Li
 * Copyright (C) Vino
 */
#include <stdio.h>
#include "vn_palloc.h"
#include "vn_alloc.h"
#include "vn_logger.h"

static void *vn_palloc_small(vn_pool_t *pool, size_t size);
static void *vn_palloc_large(vn_pool_t *pool, size_t size);
static void *vn_palloc_block(vn_pool_t *pool, size_t size);

vn_pool_t *vn_create_pool(size_t size) {
    vn_pool_t *p;

    p = vn_alloc(size);
    if (NULL == p) {
        return NULL;
    }

    p->data.last = (char *) p + sizeof(vn_pool_t);
    p->data.end = (char *) p + size;
    p->data.next = NULL;
    p->data.failed = 0;

    size = size - sizeof(vn_pool_t);
    p->max = (size < VN_MAX_ALLOC_FROM_POOL) ? size : VN_MAX_ALLOC_FROM_POOL;
    p->current = p;
    p->large = NULL;

    return p;
}

void vn_destroy_pool(vn_pool_t *pool) {
    vn_pool_t *p, *n;
    vn_pool_large_t *l;

    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            vn_free(l->alloc);
        }
    }

    for (p = pool, n = p->data.next; /* void */; p = n, n = n->data.next) {
        vn_free(p);
        
        if (NULL == n) {
            break;
        }
    }
}

void vn_reset_pool(vn_pool_t *pool) {
    vn_pool_t *p;
    vn_pool_large_t *l;
    
    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            vn_free(l->alloc);
        }
    }

    for (p = pool; p; p = p->data.next) {
        p->data.last = (char *) p + sizeof(vn_pool_t);
        p->data.failed = 0;
    }

    pool->current = pool;
    pool->large = NULL;
}

void *vn_palloc(vn_pool_t *pool, size_t size) {
    if (size <= pool->max) {
        return vn_palloc_small(pool, size);
    }
    return vn_palloc_large(pool, size);
}

static void *vn_palloc_small(vn_pool_t *pool, size_t size) {
    char *m;
    vn_pool_t *p;

    p = pool->current;
    do {
        m = p->data.last;
        if ((size_t) (p->data.end - m) >= size) {
            p->data.last += size;
            return m;
        }
        p = p->data.next;
    } while (p);

    return vn_palloc_block(pool, size);
}

static void *vn_palloc_block(vn_pool_t *pool, size_t size) {
    char *m;
    size_t psize;
    vn_pool_t *p, *new;

    psize = pool->data.end - (char *) pool;

    m = vn_alloc(psize);
    if (NULL == m) {
        return NULL;
    }

    new = (vn_pool_t *)m;

    new->data.end = m + psize;
    new->data.next = NULL;
    new->data.failed = 0;

    m += sizeof(vn_pool_t);
    new->data.last = m + size;

    for (p = pool->current; p->data.next; p = p->data.next) {
        if (p->data.failed++ > 4) {
            pool->current = p->data.next;
        }
    }

    p->data.next = new;
    return m;
}

static void *vn_palloc_large(vn_pool_t *pool, size_t size) {
    void *p;
    int n;
    vn_pool_large_t *large;

    p = vn_alloc(size);
    if (NULL == p) {
        return NULL;
    }

    n = 0;
    for (large = pool->large; large; large = large->next) {
        if (NULL == large->alloc) {
            large->alloc = p;
            return p;
        }
        
        if (n++ > 3) {
            break;
        }
    }

    large = vn_palloc_small(pool, sizeof(vn_pool_large_t));
    if (NULL == large) {
        vn_free(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;
    return p;
}

void vn_print_pool(vn_pool_t *pool) {
    vn_pool_data_t data;
    vn_pool_large_t *l;
    vn_pool_t *p;
    int i;

    i = 0;
    for (p = pool; p; p = p->data.next) {
        printf("pool = %p, data = %ld, remain = %ld, failed = %d\n", 
        p,  
        p->data.last - (char *) p - sizeof(vn_pool_t), 
        p->data.end - p->data.last, 
        p->data.failed);
    }

    for (l = pool->large; l; l = l->next) {
        printf("alloc: %p\n", l->alloc);
    }
}