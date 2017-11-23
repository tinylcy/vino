#include <stdio.h>
#include <stdlib.h>
#include "../src/vn_priority_queue.h"
#include "../src/error.h"

int main(void) {
    vn_pq_init();
    vn_pq_node node1 = { 3, NULL };
    vn_pq_node node2 = { 2, NULL };
    vn_pq_node node3 = { 5, NULL };
    vn_pq_node node4 = { 8, NULL };
    vn_pq_node node5 = { 1, NULL };
    vn_pq_node node6 = { 4, NULL };
    vn_pq_node node7 = { 2, NULL };

    vn_pq_insert(node1);
    vn_pq_insert(node2);
    vn_pq_insert(node3);
    vn_pq_insert(node4);
    vn_pq_insert(node5);
    vn_pq_insert(node6);
    vn_pq_insert(node7);

    int i;
    for (i = 0; i < 7; i++) {
        vn_pq_node node = vn_pq_delete_min();
        printf("key = %ld\n", node.key);
    }
   
    return 0;
}