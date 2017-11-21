#include <stdio.h>
#include <stdlib.h>

#include "../src/error.h"
#include "../src/util.h"

int main(void) {
    char *conf_file = "../src/vino.conf";
    vn_conf conf;

    vn_parse_config(conf_file, &conf);
    return 0;
}