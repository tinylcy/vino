#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http_headers_parser.h"
#include "rio.h"

/*-------------------------------------------------------*
	parse the headers of HTTP requset
  -------------------------------------------------------*/
struct http_request_headers* parse_headers(int fd) {
	struct http_request_headers *headers = (struct http_request_headers*)malloc(sizeof(struct http_request_headers));
	if(headers == NULL) {
		perror("malloc");
		return NULL;
	}

	char header_buf[BUFSIZ];    /* HTTP header */
	// char host_buf[BUFSIZ];    /* HTTP host */
	// char user_agent_buf[BUFSIZ];    /* HTTP user-agent */
	
	rio_t rio;
	rio_readinitb(&rio, fd);
	if(rio_readlineb(&rio, header_buf, BUFSIZ) != -1) {
		headers = parse_method_path_version(header_buf, headers);
	} else {
		perror("rio_readlineb");
	}
	
	return headers;

}

/*-------------------------------------------------------*
	parse the first line of HTTP request to fetch 
	HTTP method, HTTP path and HTTP version.
  -------------------------------------------------------*/
struct http_request_headers* parse_method_path_version(char *header_buf, struct http_request_headers *headers) {

	char method[BUFSIZ], uri[BUFSIZ], version[BUFSIZ];
	sscanf(header_buf, "%s %s %s\r\n", method, uri, version);
	
	headers->method = (char*)malloc(sizeof(char) * (strlen(method) + 1));
	memset(headers->method, 0, strlen(method) + 1);    /* initialize the array to 0 */
	strcpy(headers->method, method);
	
    	headers->uri = (char*)malloc(sizeof(char) * (strlen(uri) + 1));
	memset(headers->uri, 0, strlen(uri) + 1);
	strcpy(headers->uri, uri);

	headers->version = (char*)malloc(sizeof(char) * (strlen(version) + 1));
	memset(headers->version, 0, strlen(version) + 1);
	strcpy(headers->version, version);
	
	return headers;
}


void read_until_crnl(FILE *fp) {
	char buf[BUFSIZ];
	while(fgets(buf, BUFSIZ, fp) != NULL && strcmp(buf, "\r\n") != 0) {
		;
	}
}
