/*
 * Copyright (C) Chenyang Li
 * Copyright (C) Vino
 */
#ifndef VINO_VN_ALLOC_H
#define VINO_VN_ALLOC_H

#include <stdio.h>

void *vn_alloc(size_t size);
void *vn_calloc(size_t size);
void vn_free(void *p);

#endif /* VINO_VN_ALLOC_H */