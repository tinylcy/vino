/*
 * HTTP Request Headers
 */
typedef struct http_request_headers {
	char *method;
	char *uri;
	char *query_args;
	char *version;
	char *host;
	char *user_agent;
	char *post_data;
} http_request_headers_t;

void init_headers(http_request_headers_t *headers);

http_request_headers_t* parse_headers(int fd);

http_request_headers_t* parse_method_uri_version(char *buf, http_request_headers_t *headers);

http_request_headers_t* parse_post_data(int fd, http_request_headers_t *headers);

void read_until_crnl(int);

void http_request_free(http_request_headers_t *headers);
