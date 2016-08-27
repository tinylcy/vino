void process_request(char*, int);
void read_until_crnl(FILE *);
void header(FILE*, char*);

char *file_type(char*);

void cannot_execute(int);
void do_404(char*, int);
void do_ls(char*, int);
void do_exec(char*, int);
void do_cat(char *, int);

int isdir(char*);
int not_exist(char*);
int ends_in_cgi(char*);


