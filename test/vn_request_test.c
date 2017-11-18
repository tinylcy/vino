#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#include "../vn_request.h"
#include "../rio.h"
#include "../error.h"

#define BUFSIZE 1024

int main(void) {
    char buf[BUFSIZE];
    int fd, rv;
    rio_t rio;

    if ((fd = open("get_request", O_RDWR)) < 0) {
        err_sys("[vn_request_test] open error");
    }
    
    rio_readinitb(&rio, fd);
    rio_readnb(&rio, buf, BUFSIZE);
    // printf("%s", buf);
    
    vn_http_request hr;
    rv = vn_parse_http_request(buf, strlen(buf), &hr);

    // printf("rv = %d\n", rv);
    print_http_request(&hr);

    return 0;
}