#include <stdio.h>
#include <stdlib.h>
#include "../src/util.h"

int main(void) {
    int rv;
    vn_str str1;
    char *str2;

    str1.p = "chenyang";
    str1.len = 8;
    str2 = "chenyang";

    rv = vn_str_cmp(&str1, str2);
    printf("%s cmp %s = %d\n", str1.p, str2, rv);

    str2 = "chenyangli";

    rv = vn_str_cmp(&str1, str2);
    printf("%s cmp %s = %d\n", str1.p, str2, rv);

    str1.p = "chenyangll";
    str1.len = 10;

    rv = vn_str_cmp(&str1, str2);
    printf("%s cmp %s = %d\n", str1.p, str2, rv);

    return 0;

}