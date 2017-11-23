/*
 *  Copyright (C) Chenyang Li
 *  Copyright (C) Vino
 */
#ifndef VINO_VN_PRIORITY_QUEUE_H
#define VINO_VN_PRIORITY_QUEUE_H

#include <time.h>
#include "vino.h"

#define VN_MAX_PQ_SIZE 8192

typedef struct vn_pq_node_s {
    time_t          key;
    vn_http_event   *data;
} vn_pq_node;

/* 
 * Priority queue (not using the first entry).
 * 
 * The parent of the node in position k is in position k/2 and,
 * conversely, the two children of the node in the position k
 * are in positions 2k and 2k + 1.
 */
static vn_pq_node *pq = NULL;

/* The number of nodes in priority queue */
static unsigned int size = 0;

/*
 * Create a priority queue.
 */
void vn_pq_init(void);

/*
 * Insert a node into priority queue.
 */
void vn_pq_insert(vn_pq_node node);

/*
 * Return and remove the smallest node;
 */
vn_pq_node vn_pq_delete_min(void);

/*
 * Is the priority queue empty?
 */ 
int vn_pq_isempty(void);

/*
 * Number of nodes in the priority queue.
 */ 
unsigned int vn_pq_size(void);

#endif /* VINO_VN_PRIORITY_QUEUE_H */