#include <stdio.h>
#include <stdlib.h>
#include "../src/util.h"
#include "../src/vn_error.h"

int main(void) {
    char *filepath = "/index.html";
    char mime_type[50];

    vn_get_mime_type(filepath, mime_type);
    printf("filepath = %s, mime_type = %s\n", filepath, mime_type);

    filepath = "index.";
    vn_get_mime_type(filepath, mime_type);
    printf("filepath = %s, mime_type = %s\n", filepath, mime_type);

    filepath = "index";
    vn_get_mime_type(filepath, mime_type);
    printf("filepath = %s, mime_type = %s\n", filepath, mime_type);

    filepath = "index.pdf";
    vn_get_mime_type(filepath, mime_type);
    printf("filepath = %s, mime_type = %s\n", filepath, mime_type);

    filepath = "index.html.html";
    vn_get_mime_type(filepath, mime_type);
    printf("filepath = %s, mime_type = %s\n", filepath, mime_type);
    
    return 0;
}