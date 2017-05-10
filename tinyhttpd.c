/*
 * Copyright (C) Chenyang Li
 * Copyright (C) tinyhttpd
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
#include <errno.h>
#include <dirent.h>
#include "tinyhttpd.h"
#include "socketlib.h"
#include "rio.h"
#include "util.h"
#include "error.h"
#include "debug.h"
#include "threadpool.h"
#include "http_epoll.h"

#define error(msg) { perror(msg); }
#define CONFIG_FILE_NAME "tinyhttpd.conf"

time_t server_started;
int server_bytes_sent;
int server_requests;

int main(int ac, char *av[]) {
	int listenfd = -1;
	int fd;
	// int *fdptr;

	struct httpd_conf conf;

	if (ac != 1 && ac != 2) {
		fprintf(stderr, "usage: ./tinyhttpd & || ./tinyhttpd port &\n");
		return 0;
	}

	if (ac == 1) {
		init_conf(&conf);
		listenfd = make_server_socket(conf.port);
	} else if (ac == 2) {
		listenfd = make_server_socket(atoi(av[1]));
	}

	if (listenfd == -1) {
		log_err("fail to make server socket.");
		exit(EXIT_FAILURE);
	}

	if (make_socket_non_blocking(listenfd) != 0) {
		log_err("fail to make socket non_blocking.");
		exit(EXIT_FAILURE);
	}

	if (conf.thread_num == 0 || conf.job_max_num == 0) {
		log_err("the thread_num is 0 or the job_max_num is 0.");
		exit(EXIT_FAILURE);
	}

	// threadpool_t *pool = threadpool_init(conf.thread_num, conf.job_max_num);
	// if(pool == NULL) {
	//	log_err("fail to initialize the threadpool.");
	//	exit(EXIT_FAILURE);
	// }

	/*
	 * create epoll and add listenfd to interest list
	 */
	int epfd = http_epoll_create(0);
	printf("epfd: %d\n", epfd);
	http_request_t *request = NULL;
	request = (http_request_t *)malloc(sizeof(http_request_t));
	if (request == NULL) {
		log_err("fail to malloc http_request_t.");
		return -1;
	}
	http_request_init(request, listenfd, epfd);

	struct epoll_event event;
	event.data.ptr = (void *)request;
	// event.data.fd = listenfd;
	event.events = EPOLLIN | EPOLLET;
	http_epoll_add(epfd, listenfd, &event);
	log_info("success to add fd: %d into interest list.", listenfd);

	int i, n;

	printf("\ntinyhttpd started successfully!\n");

	while (1) {

		n = http_epoll_wait(epfd, evlist, MAXEVENTS, -1);
		if (n == -1) {
			if (errno == EINTR) {
				continue;
			}
			log_err("epoll wait error.");
			return -1;
		}

		for (i = 0; i < n; i++) {
			http_request_t *req = (http_request_t *)evlist[i].data.ptr;
			fd = req->fd;
			log_info("ready fd: %d.", fd);
			if (fd == listenfd) {
				/*
				 * we have a notification on the listening socket,
				 * which means one or more incoming connections.
				 */
				int connfd;
				while (1) {
					connfd = accept(listenfd, NULL, NULL);
					if (connfd < 0) {
						if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
							break;
						} else {
							log_err("fail to accept a connfd.");
							break;
						}
					}

					log_info("success to accept a fd: %d.", connfd);

					if (make_socket_non_blocking(connfd) != 0) {
						log_err("fail to make socket non-blocking: connfd = %d.", connfd);
					}

					http_request_t *r = (http_request_t *)malloc(sizeof(http_request_t));
					http_request_init(r, connfd, epfd);
					// event.data.fd = connfd;
					event.data.ptr = (void *)r;
					event.events = EPOLLIN | EPOLLET;
					http_epoll_add(epfd, connfd, &event);
					log_info("success to add fd: %d into interest list.", connfd);

				}

			} else {

				if (evlist[i].events & EPOLLIN) {
					/*
					 * in this case, we have to malloc an
					 * extra space to store fd, CASPP page 661
					 * explains the reason.
					 */
					// fdptr = (int *)malloc(sizeof(int));
					// *fdptr = fd;

					// threadpool_add_job(pool, handle, fdptr);
					// log_info("success to add a job into threadpool: fd = %d.", fd);
					// fdptr = NULL;

					//threadpool_add_job(pool, do_request, evlist[i].data.ptr);

					process_request(evlist[i].data.ptr);

				} else if (evlist[i].events & (EPOLLHUP | EPOLLERR)) {
					close(fd);
					log_info("success to close fd: %d", fd);
				}
			}
		}
	}

	printf("exit tinyhttpd.");

	return 0;
}

/*-----------------------------------------------------------*
	initialize the configuration
  -----------------------------------------------------------*/
void init_conf(struct httpd_conf *conf) {
	FILE *fp = NULL;
	char line[BUFSIZ];
	char param_name[BUFSIZ];
	char param_value[BUFSIZ];

	if ((fp = fopen(CONFIG_FILE_NAME, "r")) == NULL) {
		perror("fail to open the configuration file.");
		exit(EXIT_FAILURE);
	}

	while (fgets(line, BUFSIZ, fp) != NULL) {
		sscanf(line, "%s %s", param_name, param_value);
		if (strcmp(param_name, PORT) == 0) {
			conf->port = atoi(param_value);
		}
		if (strcmp(param_name, THREAD_NUM) == 0) {
			conf->thread_num = atoi(param_value);
		}
		if (strcmp(param_name, JOB_MAX_NUM) == 0) {
			conf->job_max_num = atoi(param_value);
		}
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

void process_request(void *req_ptr) {
	http_request_t *request = (http_request_t *)req_ptr;
	int fd = request->fd;
	char *cursor = NULL;
	size_t remain_size = 0;
	int n, ret;

	while (1) {
		cursor = &request->buf[request->last % REQ_MAX_BUF];
		remain_size = 200;

		n = read(fd, cursor, remain_size);

		if (n == 0) { // EOF
			break;
		}
		if (n < 0) {
			if (errno == EAGAIN) {
				break;
			}
			break;
		}

		request->last += n;

		ret = http_request_parse(request);
		if (ret == HTTP_PARSE_AGAIN) {
			continue;
		} else if (ret == HTTP_PARSE_OK) {
			http_response(request);
			break;
		}

	}

	// close(fd);
}

void http_response(http_request_t *request) {
	char method[BUFSIZ];
	char uri[BUFSIZ];

	memset(method, 0, BUFSIZ);
	strncpy(method, request->method_start, request->method_end - request->method_start + 1);
	memset(uri, 0, BUFSIZ);
	strcpy(uri, ".");
	strncpy(uri + 1, request->uri_start, request->uri_end - request->uri_start + 1);

	if (strcmp(method, "GET") && strcmp(method, "POST")) {
		not_implement(request->fd);
	} else if (!file_exist(uri)) {
		do_404(uri, request->fd);
	} else if (is_directory(uri)) {
		do_ls(uri, request->fd);
	} else if (is_dynamic(uri)) {
		// TODO
	} else {
		serve_static(uri, request->fd);
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
	if (content_type) {
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
	log_info("success to close fd: %d.", fd);
}

/*--------------------------------------------------------*
	no such object.
  --------------------------------------------------------*/
void do_404(char *uri, int fd) {
	// headers(fd, 404, "Not Found", "text/html");
	// char buf[BUFSIZ];
	// sprintf(buf, "<h1>The item you request:<I> %s%s </I> is not found</h1>\r\n", uri, "\0");
	// rio_writen(fd, buf, strlen(buf));
	// close(fd);
	// log_info("success to close fd: %d.", fd);

	char header[BUFSIZ], body[BUFSIZ];

	sprintf(body, "<html>");
	sprintf(body, "%s<body><h1>%s Not found.</h1>", body, uri);
	sprintf(body, "%s</body>", body);
	sprintf(body, "%s</html>", body);

	sprintf(header, "HTTP/1.1 %s %s\r\n", "404", "Not Found.");
	sprintf(header, "%sServer: tinyhttpd", header);
	sprintf(header, "%sContent-type: text/html\r\n", header);
	sprintf(header, "%sConnection: close\r\n", header);
	sprintf(header, "%sContent-length: %d\r\n\r\n", header, (int)strlen(body));

	rio_writen(fd, header, strlen(header));
	rio_writen(fd, body, strlen(body));

	// close(fd);
}


/*-------------------------------------------------------*
	when the request resource is a directory, list the
	directory to client.
  -------------------------------------------------------*/
void do_ls(char *dir, int fd) {

	DIR *dirp;
	struct dirent *dp;
	char entry[BUFSIZ];

	dirp = opendir(dir);
	if (dirp == NULL) {
		perror("opendir");
		exit(1);
	}

	headers(fd, 200, "OK", "text/plain");

	/* for each entry in this directory, print filename to client */
	for (;;) {
		errno = 0;    /* to distinguish error from end-of-directory */
		dp = readdir(dirp);
		if (dp == NULL) {
			break;
		}

		if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
			continue;    /* skip . and .. */
		}

		memset(entry, 0, BUFSIZ);
		strncpy(entry, dp->d_name, strlen(dp->d_name));
		strcat(entry, "\r\n");

		rio_writen(fd, entry, strlen(entry));
	}

	if (errno != 0) {
		log_err("fail to read DIR: %s.", dir);
		exit(EXIT_FAILURE);
	}

	if (closedir(dirp) == -1) {
		log_err("fail to close DIR: %s.", dir);
		exit(EXIT_FAILURE);
	}

	close(fd);
	log_info("success to close fd: %d.", fd);

}

/*--------------------------------------------------------*
	sanitize the command to prevent the execution
	of dangerous commands. such as 'ls ./html/;rm -rf /'
  --------------------------------------------------------*/
void sanitize(char *str) {
	char *src, *dest;
	src = str;
	dest = str;
	for ( ; *src; src++) {
		if (*src == '/' || *src == '.' || *src == '_'
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
	if ((cp = strrchr(uri, '.')) != NULL) {
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
  	serve the dynamic content. check if a cgi program is
  	executable at first, then serve the dynamic content
  	according to the HTTP method.
  -------------------------------------------------------*/
void serve_dynamic(http_request_headers_t *request, int fd) {

	char prog[BUFSIZ];
	strcpy(prog, ".");
	strcat(prog, request->uri);

	/* the file is not executable */
	if (!is_executable(prog)) {
		headers(fd, 403, "Forbidden", "text/html");
		char buf[BUFSIZ];
		sprintf(buf, "<h1>tinyhttpd couldn't run the CGI program: <I>%s</I>.</h1>\r\n", prog);
		rio_writen(fd, buf, strlen(buf));
		close(fd);
		return;
	}

	if (strcmp(request->method, "GET") == 0) {
		serve_get_dynamic(request, fd);
	} else if (strcmp(request->method, "POST") == 0) {
		serve_post_dynamic(request, fd);
	}

}

/*-------------------------------------------------------*
  	fork a new process to serve the HTTP GET dynamic
  	content.
  -------------------------------------------------------*/
void serve_get_dynamic(http_request_headers_t *request, int fd) {

	char prog[BUFSIZ];
	strcpy(prog, ".");
	strcat(prog, request->uri);

	pid_t pid;
	pid = fork();
	if (pid == -1) {
		perror("fork");
		exit(1);
	}

	if (pid == 0) {
		headers(fd, 200, "OK", "text/html");

		/* set environment variables */
		if (request->query_args != NULL) {
			setenv("QUERY_STRING", request->query_args, 1);
		} else {
			setenv("QUERY_STRING", "", 1);
		}

		dup2(fd, 1);
		dup2(fd, 2);
		close(fd);
		execl(prog, prog, NULL);
		exit(0);
	} else {
		close(fd);
	}
}

/*-------------------------------------------------------*
  	fork a new process to serve the HTTP POST dynamic
  	content.
  -------------------------------------------------------*/
void serve_post_dynamic(http_request_headers_t *request, int fd) {
	char prog[BUFSIZ];
	strcpy(prog, ".");
	strcat(prog, request->uri);

	pid_t pid;
	pid = fork();
	if (pid == -1) {
		perror("fork");
		exit(1);
	}

	if (pid == 0) {
		headers(fd, 200, "OK", "text/html");

		/* set environment variables */
		if (request->post_data != NULL) {
			setenv("POST_DATA", request->post_data, 1);
		} else {
			setenv("POST_DATA", "", 1);
		}

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
	if (!is_readable(uri)) {
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

	if (strcmp(extension, "html") == 0) {
		content_type = "text/html";
	} else if (strcmp(extension, "gif") == 0) {
		content_type = "image/gif";
	} else if (strcmp(extension, "jpg") == 0) {
		content_type = "image/jpeg";
	} else if (strcmp(extension, "jpeg") == 0) {
		content_type = "image/jpeg";
	}

	fpfile = fopen(uri, "r");

	if (fpfile != NULL) {
		headers(fd, 200, "OK", content_type);
		while ((c = getc(fpfile)) != EOF) {
			rio_writen(fd, &c, 1);
		}
		fclose(fpfile);
		close(fd);
		log_info("success to close fd: %d.", fd);
	}

}
