/*
 * Copyright (C) Chenyang Li
 * Copyright (C) Vino
 */
#include <stdlib.h>
#include <time.h>
#include "vn_priority_queue.h"
#include "vn_palloc.h"
#include "vn_logger.h"
#include "vn_error.h"

static int vn_less(vn_priority_queue_t *pq, int i, int j) {
    vn_priority_queue_node_t *a = pq->nodes[i], *b = pq->nodes[j];
    return a->key < b->key;
}

static void vn_exch(vn_priority_queue_t *pq, int i, int j) {
    vn_priority_queue_node_t *t = pq->nodes[i];
    pq->nodes[i] = pq->nodes[j];
    pq->nodes[j] = t;
}

static void vn_swim(vn_priority_queue_t *pq, int k) {
    while (k > 1 && vn_less(pq, k, k >> 1)) {
        vn_exch(pq, k, k >> 1);
        k >>= 1;
    }
}

static void vn_sink(vn_priority_queue_t *pq, int k) {
    int i;
    while (k << 1 <= pq->size) {
        i = k << 1;
        if (i < pq->size && vn_less(pq, i + 1, i)) {
            i += 1;
        }
        if (vn_less(pq, k, i)) { break; }
        vn_exch(pq, k, i);
        k = i;
    }
}

void vn_pq_init(vn_priority_queue_t *pq) {
    if ((pq->nodes = (vn_priority_queue_node_t **) 
                        malloc(VN_MAX_PQ_SIZE * sizeof(vn_priority_queue_node_t *))) == NULL) {
        err_sys("[vn_pq_init] malloc priority queue nodes error");
    }
    pq->size = 0;
}

void vn_pq_insert(vn_priority_queue_t *pq, vn_priority_queue_node_t *node) {
    if (!pq || !pq->nodes) {
        err_sys("[vn_pq_insert] the priority queue has not been initialized");
    }
    pq->nodes[++pq->size] = node;
    vn_swim(pq, pq->size);
}

vn_priority_queue_node_t *vn_pq_min(vn_priority_queue_t *pq) {
    if (!pq || !pq->nodes) {
        err_sys("[vn_pq_insert] the priority queue has not been initialized");
    }
    if (vn_pq_isempty(pq)) {
        err_sys("[vn_pq_delete_min] the priority queue is empty");
    }
    vn_priority_queue_node_t *min = pq->nodes[1];
    return min;
}

vn_priority_queue_node_t *vn_pq_delete_min(vn_priority_queue_t *pq) {
    vn_priority_queue_node_t *min = vn_pq_min(pq);
    vn_exch(pq, 1, pq->size--);
    // free(pq->nodes[pq->size + 1]);   /* Don't forget */
    vn_sink(pq, 1);
    return min;
}

vn_priority_queue_node_t *vn_pq_delete_node(vn_priority_queue_node_t *node) {
    if (NULL == node) {
        return NULL;
    }
    if (node->deleted == VN_PQ_DELETED || node->data == NULL) {
        vn_log_warn("The priority queue node has already been marked as [DELETED].");
        return node;
    }
    node->deleted = VN_PQ_DELETED;
    return node;
}

int vn_pq_isempty(vn_priority_queue_t *pq) {
    return !pq->size;
}

unsigned int vn_pq_size(vn_priority_queue_t *pq) {
    return pq->size;
}