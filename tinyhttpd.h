#define DYNAMIC_PATH "cgi-bin"

/* store the configuration params */
struct httpd_conf {
	char port[BUFSIZ];
};

int make_server_socket_q(int, int);
int make_server_socket(int);
int connect_to_server(char*, int);

void init_conf(struct httpd_conf*);
void setup(pthread_attr_t*);
void* handle(void*);
void process_request(struct http_request_headers*, int);
void header(FILE*, char*);

char *file_type(char*);

void not_implement(int);
void do_404(char*, int);
void do_ls(char*, int);
void serve_static(char *, int);
void serve_dynamic(char*, int);

void sanitize(char*);
int is_dynamic(const char*);
