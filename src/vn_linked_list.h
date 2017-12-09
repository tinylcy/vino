/*
 * Copyright (C) Chenyang Li
 * Copyright (C) Vino
 */
#ifndef VINO_VN_LINKED_LIST_H
#define VINO_VN_LINKED_LIST_H

/*
 * Linked list node structure
 */
typedef struct vn_linked_list_node_s {
    /* Pointer to the next node */
    struct vn_linked_list_node_s *next;
    /* Pointer to the previous node */
    struct vn_linked_list_node_s *prev;
    /* Pointer to data */
    void                         *data;
} vn_linked_list_node_t;

/*
 * Linked list structure 
 */ 
typedef struct vn_linked_list_s {
    /* Pointer to the first of list */
    vn_linked_list_node_t *head;
    /* Pointer to the last node of list */
    vn_linked_list_node_t *tail;
    /* Size of the linked list */
    size_t size;
} vn_linked_list_t;

/*
 * Initialize a linked list. 
 */ 
void vn_linked_list_init(vn_linked_list_t *list);

/*
 * Append data to the tail of list. 
 */ 
void vn_linked_list_append(vn_linked_list_t *list, void *data);

/* 
 * Free the linked list and all its node and data.
 */ 
void vn_linked_list_free(vn_linked_list_t *list);

/*
 * Return the size of the linked list. 
 */ 
size_t vn_linked_list_size(vn_linked_list_t *list);

/*
 * Check if the linked list is empty. 
 */ 
int vn_linked_list_isempty(vn_linked_list_t *list);

#endif /* VINO_VN_LINKED_LIST_H */