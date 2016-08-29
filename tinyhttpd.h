/*store the configuration params*/
struct httpd_conf {
	char port[BUFSIZ];
};

int make_server_socket_q(int, int);
int make_server_socket(int);
int connect_to_server(char*, int);

void init_conf(struct httpd_conf*);
void setup(pthread_attr_t*);
void* handle(void*);
void process_request(char*, int);
void read_until_crnl(FILE *);
void header(FILE*, char*);

char *file_type(char*);

void cannot_execute(int);
void do_404(char*, int);
void do_ls(char*, int);
void do_exec(char*, int);
void do_cat(char *, int);

void sanitize(char*);
int isdir(char*);
int not_exist(char*);
int ends_in_cgi(char*);
