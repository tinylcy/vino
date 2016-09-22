/*
 * tinyhttpd - a minimum-functional HTTP Server
 *
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
#include "http_headers_parser.h"
#include "tinyhttpd.h"
#include "rio.h"

#define error(msg) { perror(msg); }
#define CONFIG_FILE_NAME "tinyhttpd.conf"

time_t server_started;
int server_bytes_sent;
int server_requests;

int main(int ac, char *av[]) {
	int sock_id;
	int fd;
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

/*-----------------------------------------------------------*
	initialize the configuration
  -----------------------------------------------------------*/
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

/*-----------------------------------------------------------*
	set the type of thread
  -----------------------------------------------------------*/
void setup(pthread_attr_t *attrp) {
	pthread_attr_init(attrp);
	pthread_attr_setdetachstate(attrp, PTHREAD_CREATE_DETACHED);
	
	time(&server_started);
	server_bytes_sent = 0;
	server_requests = 0;
}

/*-----------------------------------------------------------*
	the execute method of threads
  -----------------------------------------------------------*/
void* handle(void *fdptr) {
	struct http_request_headers *headers = NULL;

	int fd;
	fd = *(int*)fdptr;
	free(fdptr);

	headers = parse_headers(fd);    /* parse the HTTP request and store the params in headers */
	process_request(headers, fd);

	return NULL;
}

/*-----------------------------------------------------------*
	handles HTTP requests, creates one thread to handle the 
	404, ls and cat, but fork a new process to handle exec. 
  -----------------------------------------------------------*/
void process_request(struct http_request_headers *headers, int fd) {

	char path[BUFSIZ];
	strcpy(path, ".");
	strcat(path, headers->path);    /* [path] is the path of request resources, start with '.' */
	
	printf("request path: %s\n", path);

	if(strcmp(headers->method, "GET") != 0) {
		cannot_execute(fd);
	} else if(not_exist(path)) {
		do_404(path, fd);
	} else if(isdir(path)) {
		do_ls(path, fd);
	} else if(ends_in_cgi(path)) {
		do_exec(path, fd);
	} else {
		do_cat(path, fd);
	}
}

/*---------------------------------------------------------*
	return the information HTTP headers about the request.
	Parameters: the connfd to print the headers on
		    the status code and the short message
		    the content type
  ---------------------------------------------------------*/
void headers(int fd, int status_code, char *short_msg, char *content_type) {
	char buf[BUFSIZ];
	sprintf(buf, "HTTP/1.1 %d %s\r\n", status_code, short_msg);
	rio_writen(fd, buf, strlen(buf));
	if(content_type) {
		sprintf(buf, "Content-type: %s\r\n", content_type);
		rio_writen(fd, buf, strlen(buf));
	}
	sprintf(buf, "\r\n");
	rio_writen(fd, buf, strlen(buf));
}

/*---------------------------------------------------------*
	unimplement HTTP command.
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
	no such object.
  --------------------------------------------------------*/
void do_404(char *path, int fd) {
	FILE *fp = fdopen(fd, "w");

	fprintf(fp, "HTTP/1.0 404 Not Found\r\n");
	fprintf(fp, "Content-type: text/plain\r\n");
	fprintf(fp, "\r\n");

	fprintf(fp, "The item you requeste: %s\r\n is not found\r\n", path);
	fclose(fp);
}

/*-------------------------------------------------------*
	the request resource is a directory.
  -------------------------------------------------------*/
int isdir(char *path) {
	struct stat info;
	return (stat(path, &info) != -1 && S_ISDIR(info.st_mode));
}

/*-------------------------------------------------------*
	the request resource does not exist.
  -------------------------------------------------------*/
int not_exist(char *path) {
	struct stat info;
	return (stat(path, &info) == -1);
} 

/*-------------------------------------------------------*
	when the request resource is a directory, list the 
	directory to client.
  -------------------------------------------------------*/
void do_ls(char *dir, int fd) {
	
	// FILE *socket_fpi,*socket_fpo;
	FILE *pipe_fp;
	char command[BUFSIZ];
	int c;

	// if((socket_fpi = fdopen(fd, "r")) == NULL) {
		// fprintf(stderr, "fdopen read");
	// }

	sanitize(dir);

	// if((socket_fpo = fdopen(fd, "w")) == NULL) {
		// fprintf(stderr, "fdopen write");
	// }

	sprintf(command, "ls %s", dir);

	// header(socket_fpo, "text/plain");
	headers(fd, 200, "OK", "text/plain");

	// fprintf(socket_fpo, "\r\n");
	// fflush(socket_fpo);

	if((pipe_fp = popen(command, "r")) == NULL) {
		error("popen");
	}

	while((c = getc(pipe_fp)) != EOF) {
		// putc(c, socket_fpo);
		rio_writen(fd, &c, 1);
	}

	pclose(pipe_fp);
	close(fd);
	// fclose(socket_fpo);
	// fclose(socket_fpi);
	
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
char* file_type(char *path) {
	char *cp;
	if((cp = strrchr(path, '.')) != NULL) {
		return cp + 1;
	}
	return "";
}

/*-------------------------------------------------------*
	determine whether the type of request resource is 
	cgi.
  -------------------------------------------------------*/
int ends_in_cgi(char *path) {
	return (strcmp(file_type(path), "cgi") == 0);
}

/*-------------------------------------------------------*
  fork a new process to provide dynamic content.
  -------------------------------------------------------*/
void do_exec(char *prog, int fd) {
	pid_t pid;

	pid = fork();
	if(pid == -1) {
		perror("fork");
		exit(1);	
	}

	if(pid == 0) {
		headers(fd, 200, "OK", NULL);
		dup2(fd, 1);
		dup2(fd, 2);
		close(fd);
		execl(prog, prog, NULL);
		exit(0);
	} else {
		close(fd);
	}
}

/*--------------------------------------------------------*
  send back contents after a header
  --------------------------------------------------------*/
void do_cat(char *path, int fd) {
	
	char *extension = file_type(path);
	char *content_type = "text/plain";
	FILE *fpfile;
	int c;

	if(strcmp(extension, "html") == 0) {
		content_type = "text/html";
	} else if(strcmp(extension, "gif") == 0) {
		content_type = "image/gif";
	} else if(strcmp(extension, "jpg") == 0) {
		content_type = "image/jpeg";
	} else if(strcmp(extension, "jpeg") == 0) {
		content_type = "image/jpeg";
	}
	
	fpfile = fopen(path, "r");
	
	if(fpfile != NULL) {
		headers(fd, 200, "OK", content_type);
		while((c = getc(fpfile)) != EOF) {
			rio_writen(fd, &c, 1);
		}
		fclose(fpfile);
		close(fd);
	}

}

