/*
 * HTTP Request Headers
 */
struct http_request_headers {
	char *method;
	char *path;
	char *version;
	char *host;
	char *user_agent;
};

struct http_request_headers* parse_headers(int);

struct http_request_headers* parse_method_path_version(char *, struct http_request_headers*);

void read_until_crnl(FILE*);
