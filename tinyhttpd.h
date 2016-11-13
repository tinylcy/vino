#define DYNAMIC_PATH "cgi-bin"
#define PORT "PORT"
#define THREAD_NUM "THREAD_NUM"
#define JOB_MAX_NUM "JOB_MAX_NUM"

/* store the configuration params */
struct httpd_conf {
	int port;
    int thread_num;
	int job_max_num;
};

int make_server_socket_q(int, int);
int make_server_socket(int);
int connect_to_server(char*, int);

int make_socket_non_blocking(int);

void init_conf(struct httpd_conf*);
void* handle(void*);
void process_request(struct http_request_headers*, int);
void header(FILE*, char*);

char *file_type(char*);

void not_implement(int);
void do_404(char*, int);
void do_ls(char*, int);

void serve_static(char *, int);

void serve_dynamic(struct http_request_headers*, int);
void serve_get_dynamic(struct http_request_headers*, int);
void serve_post_dynamic(struct http_request_headers*, int);

void sanitize(char*);
int is_dynamic(const char*);
