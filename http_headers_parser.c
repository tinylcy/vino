#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http_headers_parser.h"

/*-------------------------------------------------------*
	parse the headers of HTTP requset
  -------------------------------------------------------*/
struct http_request_headers* parse_headers(int fd) {
	struct http_request_headers *headers = (struct http_request_headers*)malloc(sizeof(struct http_request_headers));
	if(headers == NULL) {
		perror("malloc");
		return NULL;
	}
	
	FILE *fpin = fdopen(fd, "r");
	
	if(fpin == NULL) {
		perror("fopen");
		return NULL;
	}

	char header_buf[BUFSIZ];    /* HTTP header */
	// char host_buf[BUFSIZ];    /* HTTP host */
	// char user_agent_buf[BUFSIZ];    /* HTTP user-agent */

	if(fgets(header_buf, BUFSIZ, fpin) != NULL) {
		headers = parse_method_path_version(header_buf, headers);
		read_until_crnl(fpin);
	} else {
		perror("fgets");
	}
	
	return headers;

}

/*-------------------------------------------------------*
	parse the first line of HTTP request to fetch 
	HTTP method, HTTP path and HTTP version.
  -------------------------------------------------------*/
struct http_request_headers* parse_method_path_version(char *header_buf, struct http_request_headers *headers) {

	char method[BUFSIZ], path[BUFSIZ], version[BUFSIZ];
	sscanf(header_buf, "%s %s %s\r\n", method, path, version);
	
	headers->method = (char*)malloc(sizeof(char) * strlen(method));
	strcpy(headers->method, method);
    	headers->path = (char*)malloc(sizeof(char) * strlen(path));
	strcpy(headers->path, path);
	headers->version = (char*)malloc(sizeof(char) * strlen(version));
	strcpy(headers->version, version);
	
	return headers;
}


void read_until_crnl(FILE *fp) {
	char buf[BUFSIZ];
	while(fgets(buf, BUFSIZ, fp) != NULL && strcmp(buf, "\r\n") != 0) {
		;
	}
}
