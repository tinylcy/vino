#include <stdio.h>
#include <string.h>
#include "../src/vn_linked_list.h"
#include "../src/util.h"
#include "../src/vn_error.h"

int main(void) {
    int i;
    vn_linked_list list;
    vn_str str1, str2, str3;
    vn_str *str;
    vn_linked_list_node *node;

    vn_linked_list_init(&list);

    str1.p = "str1";
    str1.len = 4;
    str2.p = "str2";
    str2.len = 4;
    str3.p = "str3";
    str3.len = 4;   
    
    vn_linked_list_append(&list, (void *) &str1);
    vn_linked_list_append(&list, (void *) &str2);
    vn_linked_list_append(&list, (void *) &str3);

    node = list.head;
    while (node) {
        str = (vn_str *) node->data;
        printf("str = %s, len = %d\n", str->p, (int) str->len);
        node = node->next;
    }

    vn_linked_list_free(&list);

    return 0;
}