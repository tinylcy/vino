#include <stdio.h>
#include <stdlib.h>
#include "../src/vn_priority_queue.h"
#include "../src/error.h"

int main(void) {
    vn_priority_queue pq;
    
    vn_pq_init(&pq);

    vn_priority_queue_node *node1 = (vn_priority_queue_node *) malloc(sizeof(vn_priority_queue_node));
    node1->key = 3;
    vn_priority_queue_node *node2 = (vn_priority_queue_node *) malloc(sizeof(vn_priority_queue_node));
    node2->key = 2;
    vn_priority_queue_node *node3 = (vn_priority_queue_node *) malloc(sizeof(vn_priority_queue_node));
    node3->key = 5;
    vn_priority_queue_node *node4 = (vn_priority_queue_node *) malloc(sizeof(vn_priority_queue_node));
    node4->key = 8;
    vn_priority_queue_node *node5 = (vn_priority_queue_node *) malloc(sizeof(vn_priority_queue_node));
    node5->key = 1;
    vn_priority_queue_node *node6 = (vn_priority_queue_node *) malloc(sizeof(vn_priority_queue_node));
    node6->key = 4;
    vn_priority_queue_node *node7 = (vn_priority_queue_node *) malloc(sizeof(vn_priority_queue_node));
    node7->key = 2;

    node1->data = node2->data = node3->data = node4->data = node5->data = node6->data = node7->data = NULL;

    vn_pq_insert(&pq, node1);
    vn_pq_insert(&pq, node2);
    vn_pq_insert(&pq, node3);
    vn_pq_insert(&pq, node4);
    vn_pq_insert(&pq, node5);
    vn_pq_insert(&pq, node6);
    vn_pq_insert(&pq, node7);

    int i;
    for (i = 0; i < 7; i++) {
        vn_priority_queue_node *node = vn_pq_delete_min(&pq);
        printf("key = %ld\n", node->key);
    }
   
    return 0;
}