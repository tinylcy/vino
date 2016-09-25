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
#include "util.h"

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

	char uri[BUFSIZ];
	strcpy(uri, ".");
	strcat(uri, headers->uri);    /* [uri] is the path of request resources, start with '.' */
	
	printf("request uri: %s\n", uri);

	if(strcmp(headers->method, "GET")) {
		not_implement(fd);
	} else if(!file_exist(uri)) {
		do_404(uri, fd);
	} else if(is_directory(uri)) {
		do_ls(uri, fd);
	} else if(is_dynamic(uri)) {
		serve_dynamic(uri, fd);
	} else {
		serve_static(uri, fd);
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
void not_implement(int fd) {
	headers(fd, 501, "Not Implemented", "text/html");
	char buf[] = "tinyhttpd does not implement this method\r\n";
	rio_writen(fd, buf, strlen(buf));
	close(fd);
}

/*--------------------------------------------------------*
	no such object.
  --------------------------------------------------------*/
void do_404(char *uri, int fd) {
	headers(fd, 404, "Not Found", "text/html");
	char buf[BUFSIZ];
	sprintf(buf, "<h1>The item you request:<I> %s%s </I> is not found</h1>\r\n", uri, "\0");
	rio_writen(fd, buf, strlen(buf));
	close(fd);
}


/*-------------------------------------------------------*
	when the request resource is a directory, list the 
	directory to client.
  -------------------------------------------------------*/
void do_ls(char *dir, int fd) {
	
	FILE *pipe_fp;
	char command[BUFSIZ];
	int c;

	headers(fd, 200, "OK", "text/plain");

	sprintf(command, "ls %s", dir);
	if((pipe_fp = popen(command, "r")) == NULL) {
		error("popen");
	}

	while((c = getc(pipe_fp)) != EOF) {
		rio_writen(fd, &c, 1);
	}

	pclose(pipe_fp);
	close(fd);
	
}

/*--------------------------------------------------------*
	sanitize the command to prevent the execution
	of dangerous commands. such as 'ls ./html/;rm -rf /'
  --------------------------------------------------------*/
void sanitize(char *str) {
	char *src, *dest;
	src= str;
	dest = str;
	for( ; *src; src++) {
		if(*src == '/' || *src == '.' || *src == '_'
					|| *src == '-' || isalnum(*src)) {
			*dest++ = *src;
		} else {
			break;		
		}
	}
	*dest = '\0';
}

/*--------------------------------------------------------*
	the cgi stuff, function to check entension and
	one to run the program.
  --------------------------------------------------------*/
char* file_type(char *uri) {
	char *cp;
	if((cp = strrchr(uri, '.')) != NULL) {
		return cp + 1;
	}
	return "";
}

/*-------------------------------------------------------*
	determine whether the type of request resource is
	dynamic.
  -------------------------------------------------------*/
int is_dynamic(const char *uri) {
	return (strstr(uri, DYNAMIC_PATH) != NULL);
}

/*-------------------------------------------------------*
  	fork a new process to provide dynamic content.
  	check if a cgi program is executable at first.
  -------------------------------------------------------*/
void serve_dynamic(char *prog, int fd) {
	/* the file is not executable */
	if(!is_executable(prog)) {
		headers(fd, 403, "Forbidden", "text/html");
		char buf[BUFSIZ];
		sprintf(buf, "<h1>tinyhttpd couldn't run the CGI program: <I>%s</I>.</h1>\r\n", prog);
		rio_writen(fd, buf, strlen(buf));
		close(fd);
		return;
	}
	
	/* the file is executable */
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
  	serve static content.
	check if a file is readable at first.
  --------------------------------------------------------*/
void serve_static(char *uri, int fd) {
	/* the file is not readable */
	if(!is_readable(uri)) {
		headers(fd, 403, "Forbidden", "text/html");
		char buf[BUFSIZ];
		sprintf(buf, "<h1>tinyhttpd couldn't read the file: <I>%s</I>.</h1>\r\n", uri);
		rio_writen(fd, buf, strlen(buf));
		close(fd);
		return;	
	}

	/* the file is readable */
	char *extension = file_type(uri);
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
	
	fpfile = fopen(uri, "r");
	
	if(fpfile != NULL) {
		headers(fd, 200, "OK", content_type);
		while((c = getc(fpfile)) != EOF) {
			rio_writen(fd, &c, 1);
		}
		fclose(fpfile);
		close(fd);
	}

}
