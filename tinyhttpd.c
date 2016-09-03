/*
 * tinyhttpd.c - a minimal web server
 *
 * usage: ./tinyhttpd portnum
 * build: gcc tinyhttpd.c socklib.c -o tinyhttpd
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "tinyhttpd.h"

#define error(msg) { perror(msg); }
#define CONFIG_FILE_NAME "tinyhttpd.conf"

time_t server_started;
int server_bytes_sent;
int server_requests;

int main(int ac, char *av[]) {
	int sock_id;
	int	fd;
	int *fdptr;
	
	struct httpd_conf conf;

	pthread_t worker;
	pthread_attr_t attr;

	if(ac != 1 && ac != 2) {
		fprintf(stderr, "usage: ./tinyhttpd & || ./tinyhttpd port &\n");
		return 0;
	}

	if(ac == 1) {
		init_conf(&conf);
		sock_id = make_server_socket(atoi(conf.port));
	} else if(ac == 2) {
		sock_id = make_server_socket(atoi(av[1]));
	}

	if(sock_id == -1) {
		error("socket");
	}
	
	setup(&attr);

	/*
	 * main loop
	 */
	while(1) {
		fd = accept(sock_id, NULL, NULL);
		server_requests++;
		fdptr = (int*)malloc(sizeof(int));
		*fdptr = fd;
		pthread_create(&worker, &attr, handle, fdptr);
	}
}

/*
 * initialize the configuration
 */
void init_conf(struct httpd_conf *conf) {
	FILE *fp;
	char line[BUFSIZ];
	char param_name[BUFSIZ];
	char port[BUFSIZ];

	if((fp = fopen(CONFIG_FILE_NAME, "r")) != NULL) {
		if(fgets(line, BUFSIZ, fp) != NULL) {
			sscanf(line, "%s %s", param_name, port);
		}
	}
	
	if(strcmp(param_name, "PORT") == 0) {
		strcpy(conf->port, port);
	}

	fclose(fp);
}

void setup(pthread_attr_t *attrp) {
	pthread_attr_init(attrp);
	pthread_attr_setdetachstate(attrp, PTHREAD_CREATE_DETACHED);
	
	time(&server_started);
	server_bytes_sent = 0;
	server_requests = 0;
}

void* handle(void *fdptr) {
	FILE *fpin;
	char request[BUFSIZ];
	int fd;
	fd = *(int*)fdptr;
	free(fdptr);

	fpin = fdopen(fd, "r");
	fgets(request, BUFSIZ, fpin);
	printf("get a call: request = %s\n", request);
	read_until_crnl(fpin);

	process_request(request, fd);

	fclose(fpin);

	return NULL;
}

/*-----------------------------------------------------------*
	read_until_crnl(FILE *)
	skip over all request info until a CRNL is seen
  -----------------------------------------------------------*/
void read_until_crnl(FILE *fp) {
	char buf[BUFSIZ];
	while(fgets(buf, BUFSIZ, fp) != NULL && strcmp(buf, "\r\n") != 0) {
		;
	}
}

/*-----------------------------------------------------------*
	process_request(char *request, int fd)
	handles request in a new process
  -----------------------------------------------------------*/
void process_request(char *request, int fd) {
	char cmd[BUFSIZ], arg[BUFSIZ];
	
	strcpy(arg, ".");
	if(sscanf(request, "%s%s", cmd, arg + 1) != 2) {
		error("sscanf");
	}

	if(strcmp(cmd, "GET") != 0) {
		cannot_execute(fd);
	} else if(not_exist(arg)) {
		do_404(arg, fd);
	} else if(isdir(arg)) {
		do_ls(arg, fd);
	} else if(ends_in_cgi(arg)) {
		do_exec(arg, fd);
	} else {
		do_cat(arg, fd);
	}
}

/*---------------------------------------------------------*
	the reply header thing: all functions need one
	if content_type is NULL, we don't need send content-type
  ---------------------------------------------------------*/
void header(FILE *fp, char *content_type) {
	fprintf(fp, "HTTP/1.1 200 OK\r\n");
	if(content_type) {
		fprintf(fp, "Content-type: %s\r\n", content_type);
	
	}
	fprintf(fp, "\r\n");
}

/*---------------------------------------------------------*
		cannot_execute(fd) --> unimplement HTTP command
  ---------------------------------------------------------*/
void cannot_execute(int fd) {
	FILE *fp = fdopen(fd, "w");

	fprintf(fp, "HTTP/1.0 501 Not implemented\r\n");
	fprintf(fp, "Content-type: text/plain\r\n");
	fprintf(fp, "\r\n");

	fprintf(fp, "That command is not yet implemented\r\n");
	fclose(fp);
}

/*--------------------------------------------------------*
	do_404(char *item, int fd) --> no such object
  --------------------------------------------------------*/
void do_404(char *item, int fd) {
	FILE *fp = fdopen(fd, "w");

	fprintf(fp, "HTTP/1.0 404 Not Found\r\n");
	fprintf(fp, "Content-type: text/plain\r\n");
	fprintf(fp, "\r\n");

	fprintf(fp, "The item you requeste: %s\r\n is not found\r\n", item);
	fclose(fp);
}

int isdir(char *f) {
	struct stat info;
	return (stat(f, &info) != -1 && S_ISDIR(info.st_mode));
}

int not_exist(char *f) {
	struct stat info;
	return (stat(f, &info) == -1);
} 

void do_ls(char *dir, int fd) {
	
	FILE *socket_fpi, *socket_fpo;
	FILE *pipe_fp;
	char command[BUFSIZ];
	int c;

	if((socket_fpi = fdopen(fd, "r")) == NULL) {
		fprintf(stderr, "fdopen read");
	}

	sanitize(dir);

	if((socket_fpo = fdopen(fd, "w")) == NULL) {
		fprintf(stderr, "fdopen write");
	}

	sprintf(command, "ls %s", dir);
	printf("command: %s\n: ", command);

	header(socket_fpo, "text/plain");
	fprintf(socket_fpo, "\r\n");
	fflush(socket_fpo);

	if((pipe_fp = popen(command, "r")) == NULL) {
		error("popen");
	}

	while((c = getc(pipe_fp)) != EOF) {
		putc(c, socket_fpo);
	}

	pclose(pipe_fp);
	fclose(socket_fpo);
	fclose(socket_fpi);
	
}

void sanitize(char *str) {
	char *src, *dest;
	src= str;
	dest = str;
	for( ; *src; src++) {
		if(*src == '/' || isalnum(*src)) {
			*dest++ = *src;
		}
	}
	*dest = '\0';
}

/*--------------------------------------------------------*
	the cgi stuff, function to check entension and
	one to run the program.
  --------------------------------------------------------*/
char* file_type(char *f) {
	char *cp;
	if((cp = strrchr(f, '.')) != NULL) {
		return cp + 1;
	}
	return "";
}

int ends_in_cgi(char *f) {
	return (strcmp(file_type(f), "cgi") == 0);
}

/*fork一个子进程，提供动态内容*/
void do_exec(char *prog, int fd) {
	FILE *fp;
	pid_t pid;

	pid = fork();
	if(pid == 0) {
		fp = fdopen(fd, "w");
		header(fp, "text/html");
		fflush(fp);

		dup2(fd, 1);
		dup2(fd, 2);
		close(fd);
		execl(prog, prog, NULL);
		perror(prog);
	}
}

/*--------------------------------------------------------*
  do_cat(char *f, int fd)
  sends back contents after a header
  --------------------------------------------------------*/
void do_cat(char *f, int fd) {
	char *extension = file_type(f);
	char *content = "text/plain";
	FILE *fpsock, *fpfile;
	int c;

	if(strcmp(extension, "html") == 0) {
		content = "text/html";
	} else if(strcmp(extension, "gif") == 0) {
		content = "image/gif";
	} else if(strcmp(extension, "jpg") == 0) {
		content = "image/jpeg";
	} else if(strcmp(extension, "jpeg") == 0) {
		content = "image/jpeg";
	}

	fpsock = fdopen(fd, "w");
	fpfile = fopen(f, "r");
	if(fpsock != NULL && fpfile != NULL) {
		header(fpsock, content);
		while((c = getc(fpfile)) != EOF) {
			putc(c, fpsock);
		}
		fclose(fpfile);
		fclose(fpsock);
	}

}

