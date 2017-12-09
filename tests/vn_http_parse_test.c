#include <stdio.h>
#include <stdlib.h>
#include "../src/vn_request.h"
#include "../src/vn_http_parse.h"

int main(void) {
    int rv;
    char buf[] = {
        'G', 'E', 'T', ' ', 
        '/', 'i', 'n', 'd', 'e', 'x', '.', 'h', 't', 'm', 'l', '?', 'a', '=', '1', '&', 'b', '=', '2', ' ',
        'H', 'T', 'T', 'P', '/', '1', '.', '1',
        '\r', '\n',
        'H', 'o', 's', 't', ':', ' ', '1', '2', '7', '.', '0', '.', '0', '.', '1',
        '\r', '\n',
        'C', 'o', 'n', 'n', 'e', 'c', 't', 'i', 'o', 'n', ':', ' ',
        'k', 'e', 'e', 'p', '-', 'a', 'l', 'i', 'v', 'e',
        '\r', '\n',
        // '\r', '\n',
        '\0'
    };
    // printf("%s", buf);

    vn_http_request hr;
    vn_init_http_request(&hr);
    hr.state = 0;
    hr.pos = buf;
    rv = vn_http_parse_request_line(&hr, buf);
    printf("parse request line, rv = %d\n", rv);

    while (1) {
        rv = vn_http_parse_header_line(&hr, buf);
        printf("parse header line, rv = %d\n", rv);
        if (rv == 0) {
            break;
        }
    }
    
    vn_print_http_request(&hr);
    
    return 0;
}