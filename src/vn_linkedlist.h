/*
 *  Copyright (C) Chenyang Li
 *  Copyright (C) Vino
 */
#ifndef VINO_VN_LINKEDLIST_H
#define VINO_VN_LINKEDLIST_H

#include <stdio.h>

/*
 * LinkedList node structure.  
 */ 
typedef struct vn_ll_node_s {
    void              *data;    /* Pointer to data */ 
    struct vn_ll_node *prev;    /* Pointer to the previous node */ 
    struct vn_ll_node *next;    /* Pointer to the next node */
} vn_ll_node;

/*
 * LinkedList structure.
 */
typedef struct vn_linkedlist_s {
    vn_ll_node *head;           /* Pointer to the head node of linkedlist */
    vn_ll_node *tail;           /* Pointer to the tail node of linkedlist */
    size_t     size;            /* The size of linkedlist */
} vn_linkedlist;

/*
 * Initialize the linkedlist `list`.
 */ 
void vn_init_linkedlist(vn_linkedlist *list);

/*
 * Append `data` to the tail of linkedlist `list`. 
 */ 
void vn_append_linkedlist(vn_linkedlist *list, void *data);

/*
 * Prepend `data` to the head of linkedlist `list`. 
 */ 
void vn_prepend_linkedlist(vn_linkedlist *list, void *data);

/*
 * Return the size of linkedlist `list`. 
 */ 
size_t vn_get_linkedlist_size(vn_linkedlist *list);

#endif /* VINO_VN_LINKEDLIST_H */