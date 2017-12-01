/*
 *  Copyright (C) Chenyang Li
 *  Copyright (C) Vino
 */
#include <stdio.h>
#include <stdlib.h>

#include "vn_linked_list.h"
#include "error.h"

void vn_linked_list_init(vn_linked_list *list) {
    list->head = list->tail = NULL;
    list->size = 0;
}

void vn_linked_list_append(vn_linked_list *list, void *data) {
    vn_linked_list_node *node;

    if ((node = (vn_linked_list_node *) malloc(sizeof(vn_linked_list_node))) == NULL) {
        err_sys("[vn_linked_list_append] malloc error");
    }

    /* If the linked list is empty */
    if (vn_linked_list_isempty(list)) {
        node->next = node->prev = NULL;
        node->data = data;
        list->head = list->tail = node;
        list->size += 1;
        return;
    }

    list->tail->next = node;
    node->prev = list->tail;
    node->next = NULL;
    node->data = data;
    list->tail = node;
    list->size += 1;
}

void vn_linked_list_free(vn_linked_list *list) {
    vn_linked_list_node *current, *current_prev;

    current = list->tail;
    current_prev = current->prev;
    while (current_prev) {
        free(current);
        current = current_prev;
        current_prev = current->prev;
    }

    free(list->head);
}

size_t vn_linked_list_size(vn_linked_list *list) {
    return list->size;
}

int vn_linked_list_isempty(vn_linked_list *list) {
    return !list->head && !list->tail && !list->size;
}