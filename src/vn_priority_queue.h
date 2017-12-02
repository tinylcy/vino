/*
 *  Copyright (C) Chenyang Li
 *  Copyright (C) Vino
 */
#ifndef VINO_VN_PRIORITY_QUEUE_H
#define VINO_VN_PRIORITY_QUEUE_H

#include <sys/time.h>

#define VN_MAX_PQ_SIZE     65535
#define VN_PQ_DELETED      1
#define VN_PQ_NOT_DELETED  0

typedef unsigned long vn_msec_t;   /* Milliseconds */

/*
 * Priority queue node structure.
 */
typedef struct vn_priority_queue_node_s {
    vn_msec_t          key;      /* Comparable key to determine priority */
    void               *data;    /* Data */
    unsigned short     deleted;  /* 1-Yes | 0-No */
} vn_priority_queue_node;

/* 
 * Priority queue (not using the first entry of `nodes`).
 * 
 * The parent of the node in position k is in position k/2 and,
 * conversely, the two children of the node in the position k
 * are in positions 2k and 2k + 1.
 * 
 * TODO: Dynamic enlarge `nodes` memory space.
 */
typedef struct vn_priority_queue_s {
    vn_priority_queue_node **nodes;    /* An array `nodes` stores all points of node */
    unsigned int           size;       /* The number of nodes in priority queue */
} vn_priority_queue;

/*
 * Initialize a priority queue.
 */
void vn_pq_init(vn_priority_queue *pq);

/*
 * Insert a node into priority queue.
 */
void vn_pq_insert(vn_priority_queue *pq, vn_priority_queue_node *node);

/*
 * Return the smallest node.
 */ 
vn_priority_queue_node *vn_pq_min(vn_priority_queue *pq);

/*
 * Return and remove the smallest node.
 */
vn_priority_queue_node *vn_pq_delete_min(vn_priority_queue *pq);

/*
 * Just set the delete flag of `node`.
 */ 
vn_priority_queue_node *vn_pq_delete_node(vn_priority_queue_node *node);

/*
 * Is the priority queue empty?
 */ 
int vn_pq_isempty(vn_priority_queue *pq);

/*
 * Number of nodes in the priority queue.
 */ 
unsigned int vn_pq_size(vn_priority_queue *pq);

#endif /* VINO_VN_PRIORITY_QUEUE_H */