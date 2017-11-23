/*
 *  Copyright (C) Chenyang Li
 *  Copyright (C) Vino
 */
#include <stdlib.h>
#include <time.h>

#include "vn_priority_queue.h"
#include "vino.h"
#include "error.h"

static int vn_less(int i, int j) {
    vn_pq_node *a = &pq[i], *b = &pq[j];
    return a->key < b->key;
}

static void vn_exch(int i, int j) {
    vn_pq_node t = pq[i];
    pq[i] = pq[j];
    pq[j] = t;
}

static void vn_swim(int k) {
    while (k > 1 && vn_less(k, k >> 1)) {
        vn_exch(k, k >> 1);
        k >>= 1;
    }
}

static void vn_sink(int k) {
    int i;
    while (k << 1 <= size) {
        i = k << 1;
        if (i < size && vn_less(i + 1, i)) {
            i += 1;
        }
        if (vn_less(k, i)) { break; }
        vn_exch(k, i);
        k = i;
    }
}

void vn_pq_init(void) {
    if ((pq = (vn_pq_node *)malloc(VN_MAX_PQ_SIZE * sizeof(vn_pq_node))) == NULL) {
        err_sys("[vn_pq_init] malloc priority queue error");
    }
    size = 0;
}

void vn_pq_insert(vn_pq_node node) {
    if (!pq) {
        err_sys("[vn_pq_insert] the priority queue has not been initialized");
    }
    pq[++size] = node;
    vn_swim(size);
}

vn_pq_node vn_pq_delete_min(void) {
    if (!pq) {
        err_sys("[vn_pq_insert] the priority queue has not been initialized");
    }
    if (vn_pq_isempty()) {
        err_sys("[vn_pq_delete_min] the priority is empty");
    }
    vn_pq_node min = pq[1];
    vn_exch(1, size--);
    vn_sink(1);
    return min;
}

int vn_pq_isempty(void) {
    return size == 0;
}

unsigned int vn_pq_size(void) {
    return size;
}