#include <stdio.h>
#include <stdlib.h>
#include "../src/vn_priority_queue.h"
#include "../src/error.h"

int main(void) {
    vn_priority_queue pq;
    vn_pq_init(&pq);
    vn_priority_queue_node node1 = { 3, NULL };
    vn_priority_queue_node node2 = { 2, NULL };
    vn_priority_queue_node node3 = { 5, NULL };
    vn_priority_queue_node node4 = { 8, NULL };
    vn_priority_queue_node node5 = { 1, NULL };
    vn_priority_queue_node node6 = { 4, NULL };
    vn_priority_queue_node node7 = { 2, NULL };

    vn_pq_insert(&pq, node1);
    vn_pq_insert(&pq, node2);
    vn_pq_insert(&pq, node3);
    vn_pq_insert(&pq, node4);
    vn_pq_insert(&pq, node5);
    vn_pq_insert(&pq, node6);
    vn_pq_insert(&pq, node7);

    int i;
    for (i = 0; i < 7; i++) {
        vn_priority_queue_node node = vn_pq_delete_min(&pq);
        printf("key = %ld\n", node.key);
    }
   
    return 0;
}