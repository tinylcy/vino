/*
 * Copyright (C) Chenyang Li
 * Copyright (C) Vino
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include "vino.h"
#include "rio.h"
#include "vn_socketlib.h"
#include "vn_request.h"
#include "vn_epoll.h"
#include "vn_linked_list.h"
#include "vn_http_parse.h"
#include "vn_event_timer.h"
#include "vn_logger.h"
#include "util.h"
#include "vn_error.h"

static char *port = VN_PORT;

static const struct option long_options[] = {
    { "port", required_argument, NULL, 'p' },
    { "help", no_argument, NULL, 'h' },
    { "version", no_argument, NULL, 'V' },
    { NULL, 0, NULL, 0 }
};

static void vn_usage(char *program) {
    fprintf(stderr,
            "Usage: %s [option]\n"
            "Options are:\n"
            "    -p|--port <port>   Specify port for Vino. Default 8080.\n"
            "    -h|--help          This information.\n"
            "    -V|--version       Display program version.\n",
            program
    );
}

static void vn_parse_options(int argc, char *argv[]) {
    int opt = 0;
    int options_index = 0;

    if (1 == argc) {
        return;
    }

    while ((opt = getopt_long(argc, argv, "Vp:?h", long_options, &options_index)) != EOF) {
        switch (opt) {
            case  0 : break;
            case 'p': port = optarg; break;
            case 'V': printf(VINO_VERSION"\n"); exit(0);
            case 'h':
            case '?': vn_usage(argv[0]); exit(0);
            default : break;
        }
    }
}

/*
 * Register a handler for signal SIGUSR1.
 * 
 * Type `kill -USR1 <PID>` to trigger this handler,
 * the program will exit normally. 
 */
static void vn_sig_usr1_handler(int signum) {
    if (signum != SIGUSR1) {
        err_sys("[vn_sig_usr1_handler] signum must be SIGUSR1");
    }
    vn_log_info("Exiting on SIGUSR1...");
    exit(0);
}

static void vn_print_http_request(vn_http_request *hr);

int main(int argc, char *argv[]) {
    int rv, epfd, listenfd;
    int i, nready, fd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; 
    struct epoll_event ep_event;
    vn_http_connection *http_conn, *conn;
    vn_msec_t time;

    vn_parse_options(argc, argv);

    /* 
     * Install signal handler for SIGPIPE.
     * When a HTTP response contains header `Connection: close`,
     * the browser will close the socket after receive data, which
     * will create a SIGPIPE signal to the process.
     */
    vn_signal(SIGPIPE, SIG_IGN);

    /* 
     * Intall signal handler for SIGUSR1. 
     * When use Gprof to analyze performance, the program 
     * should exit normally. 
     */
    vn_signal(SIGUSR1, vn_sig_usr1_handler);

    if ((listenfd = open_listenfd(port)) < 0) {
        err_sys("[main] open_listenfd error");
    }
    vn_log_info("Listen file descriptor [%d] opened.", listenfd);

    if ((rv = make_socket_non_blocking(listenfd)) != 0) {
        err_sys("[main] make_socket_non_blocking error");
    }

    vn_epoll_init();

    /* Create an epoll instance referred by `epfd` */
    if ((epfd = vn_epoll_create()) < 0) {
        err_sys("[main] vn_epoll_create error");
    }

    if ((http_conn = (vn_http_connection *) malloc(sizeof(vn_http_connection))) == NULL) {
        err_sys("[main] malloc vn_http_connection error");
    }
    vn_init_http_connection(http_conn, listenfd, epfd);

    ep_event.events = EPOLLIN | EPOLLET;
    ep_event.data.ptr = (void *) http_conn;
    vn_epoll_add(epfd, listenfd, &ep_event);

    vn_event_timer_init();
    vn_log_info("Timer initialized.");

    vn_log_info("Server started, Vino version "VINO_VERSION".");
    vn_log_info("The server is now ready to accept connections on port %s.", port);

    while (VN_RUNNING) {
        time = vn_event_find_timer();
        nready = vn_epoll_wait(epfd, events, VN_MAXEVENTS, time);

        if (nready < 0) {
            if (errno == EINTR) {
                continue;   /* Restart if interrupted by signal */
            } else {
                err_sys("[main] vn_epoll_wait error");
            }
        }

        vn_event_time_update();
        vn_event_expire_timers();

        /* Deal with returned list of events */
        for (i = 0; i < nready; i++) {
            conn = (vn_http_connection *) events[i].data.ptr;
            fd = conn->fd;

            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)
                || !(events[i].events & EPOLLIN)) {
                vn_log_warn("An error has occured on file descriptor [%d], or the socket is not ready for reading.", fd);
                vn_pq_delete_node(conn->pq_node);
                vn_close_http_connection(events[i].data.ptr);
                continue;
            }

            /*
             * A notification has occured on the listening socket, which
             * means one or more incoming connections. 
             */
            if (listenfd == fd) {
                while (VN_ACCEPT) {
                    clientlen = sizeof(struct sockaddr_storage);                
                    connfd = accept(listenfd, (struct sockaddr *) &clientaddr, &clientlen);
                    
                    if (connfd < 0) {
                        if (errno == EINTR) {
                            continue;
                        } else if (errno == EAGAIN) {
                            break;  /* All incoming connections have been processed */
                        } else {
                            err_sys("[main] accept error");
                        }
                    }

                    if ((rv = make_socket_non_blocking(connfd)) != 0) {
                        err_sys("[main] make_socket_non_blocking error");
                    }

                    vn_http_connection *new_conn;
                    struct epoll_event new_ep_ev;

                    if ((new_conn = (vn_http_connection *) malloc(sizeof(vn_http_connection))) == NULL) {
                        err_sys("[main] malloc vn_http_connection error");
                    }

                    vn_init_http_connection(new_conn, connfd, epfd);
                    new_ep_ev.events = EPOLLIN | EPOLLET;
                    new_ep_ev.data.ptr = (void *) new_conn;
                    vn_epoll_add(epfd, connfd, &new_ep_ev);

                    vn_event_add_timer(new_conn, VN_DEFAULT_TIMEOUT);
                    
                }
            /* End of fd == listenfd */

            } else {
                vn_handle_http_connection((vn_http_connection *) events[i].data.ptr);
            }
        }
    }

    return 0;
}

void vn_handle_http_connection(vn_http_connection *conn) {
    int nread, buf_len;
    int rv, req_line_completed, req_headers_completed;

    while (VN_KEEP_READING) {
        nread = read(conn->fd, conn->bufptr, conn->remain_size);

        /* End of file, the remote has closed the connection. */
        if (0 == nread) {
            break;
        }

        /* If errno == EAGAIN, that means we have read all data. */
        if (nread < 0) {
            if (errno == EINTR) {
                continue;
            } if (errno != EAGAIN && errno != EWOULDBLOCK) {
                err_sys("[vn_handle_http_connection] rio_readn error");
            }
            break;
        }

        conn->bufptr += nread;
        conn->request.last += nread;     /* Update the last character parser can read */
        conn->remain_size -= nread;

        rv = vn_http_parse_request_line(&conn->request, conn->buf);
        if (rv < 0) {
            /* 
             * Before closing the illegal connection, the corresponding
             * node should be removed in priority queue at first, or
             * after timeout the file descriptor will be closed twice.
             */
            vn_pq_delete_node(conn->pq_node);
            vn_close_http_connection((void *) conn);
            vn_log_warn("A connection with illegal HTTP request line has been closed.");
            return;
        }
        if (rv == VN_AGAIN) {
            continue;
        } else {
            req_line_completed = VN_OK;
        }

        while (VN_KEEP_PARSING) {
            rv = vn_http_parse_header_line(&conn->request, conn->buf);
            if (rv < 0) {
                vn_pq_delete_node(conn->pq_node);
                vn_close_http_connection((void *) conn);
                vn_log_warn("A connection with illegal HTTP request headers has been closed.");
                return;
            }
            if (rv == VN_HTTP_PARSE_HEADER_DONE) {
                continue;
            } else if (rv == VN_AGAIN) {
                break;
            } else if (rv == VN_OK) {
                req_headers_completed = VN_OK;
                break;
            }
        }

    }

    if (!(req_line_completed == VN_OK && req_headers_completed == VN_OK)) {
        return;
    }

    // vn_print_http_request(&conn->request);

    if (!vn_str_cmp(&conn->request.method, "GET")) {
        vn_handle_get_connection(conn);
    } else if (!vn_str_cmp(&conn->request.method, "POST")) {
        // TODO
    }

}

void vn_handle_get_connection(vn_http_connection *conn) {
    vn_http_request *req;
    char uri[VN_MAX_HTTP_HEADER_VALUE], filepath[VN_MAX_HTTP_HEADER_VALUE];

    /* 
     * Remove the corresponding node in priority queue. 
     * This is because the connection will be dealt with soon.
     */
    vn_pq_delete_node(conn->pq_node);

    req = &conn->request;
    if (vn_get_string(&req->uri, uri, VN_MAX_HTTP_HEADER_VALUE) == -1) {
        err_sys("[vn_handle_get_connection] vn_get_string error");
    }
    if (strlen(uri) == 1 && !strncmp(uri, "/", 1)) {
        strncpy(uri, VN_DEFAULT_PAGE, strlen(VN_DEFAULT_PAGE));
    }

    /* Append default static resource path before HTTP request's uri */
    memset(filepath, '\0', VN_MAX_HTTP_HEADER_VALUE);
    if (strcat(filepath, VN_PARENT_DIR) == NULL) {
        err_sys("[vn_handle_get_connection] strcat [VN_PARENT_DIR] error");
    }
    if (strcat(filepath, VN_DEFAULT_STATIC_RES_DIR) == NULL) {
        err_sys("[vn_handle_get_connection] strcat [DEFAULT_STATIC_RES_DIR] error");
    }
    if (strncat(filepath, uri, strlen(uri)) == NULL) {
        err_sys("[vn_handle_get_connection] strcat [uri] error");
    }

    char headers[VN_HEADERS_SIZE], body[VN_BODY_SIZE];
    int srcfd;
    void *srcp;
    unsigned int filesize;
    char mime_type[VN_FILETYPE_SIZE];
    vn_str *conn_str;
    short conn_flag;
    
    // TODO: nwrite = rio_writen(fd, buf, size); Check if `nwrite` == `size`.
    if (vn_check_file_exist(filepath) < 0) {
        vn_build_resp_404_body(body, uri);
        vn_build_resp_headers(headers, 404, "Not Found", "text/html", strlen(body), VN_CONN_CLOSE);
        rio_writen(conn->fd, (void *) headers, strlen(headers));
        rio_writen(conn->fd, (void *) body, strlen(body));
        vn_close_http_connection((void *) conn);
        return;
    } 
    
    if (vn_check_read_permission(filepath) < 0) {
        vn_build_resp_403_body(body, uri);
        vn_build_resp_headers(headers, 403, "Forbidden", "text/html", strlen(body), VN_CONN_CLOSE);
        rio_writen(conn->fd, (void *) headers, strlen(headers));
        rio_writen(conn->fd, (void *) body, strlen(body));
        vn_close_http_connection((void *) conn);
        return;
    }

    if ((srcfd = open(filepath, O_RDONLY, 0)) < 0) {
        err_sys("[vn_handle_get_connection] open error");
    }
    filesize = vn_get_filesize(filepath);
    /* Map the target file into memory */
    if ((srcp = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0)) == MAP_FAILED) {
        vn_log_warn("Call mmap error, filepath = %s\n", filepath);
        err_sys("[vn_handle_get_connection] mmap error");
    }
    if (close(srcfd) < 0) {
        err_sys("[vn_handle_get_connection] close srcfd error");
    }

    /* Build response header by Connection header */
    vn_get_mime_type(filepath, mime_type);
    conn_str = vn_get_http_header(req, "Connection");
    if (NULL != conn_str && (!vn_str_cmp(conn_str, "keep-alive") || !vn_str_cmp(conn_str, "Keep-Alive"))) {
        vn_build_resp_headers(headers, 200, "OK", mime_type, filesize, VN_CONN_KEEP_ALIVE);
        conn_flag = VN_CONN_KEEP_ALIVE;
    } else {
        vn_build_resp_headers(headers, 200, "OK", mime_type, filesize, VN_CONN_CLOSE);
        conn_flag = VN_CONN_CLOSE;
    }

    /* Send response */
    rio_writen(conn->fd, headers, strlen(headers));
    rio_writen(conn->fd, srcp, filesize);
    if (munmap(srcp, filesize) < 0) {
        err_sys("[vn_handle_get_connection] munmap error");
    }

    /* 
     * If persistent connection flag (Connection: keep-alive) is set,
     * several connections will share the same buffer, therefore unparsed
     * data in buffer should be moved to the beginning of buffer.
     */
    if (VN_CONN_KEEP_ALIVE == conn_flag) {
        /* Restart timer */
        vn_event_add_timer(conn, VN_DEFAULT_TIMEOUT);
        /* Reset HTTP connection */
        vn_reset_http_connection(conn);
    } else {
        vn_close_http_connection((void *) conn);
    }

}

void vn_build_resp_headers(char *headers, 
                           int code, 
                           const char *reason, 
                           const char *content_type, 
                           unsigned int content_length, 
                           short keep_alive) {
    char *conn;
    if (NULL == reason) {
        reason = vn_status_message(code);
    }
    conn = (keep_alive == VN_CONN_KEEP_ALIVE) ? "keep-alive" : "close";
    sprintf(headers, "HTTP/1.1 %d %s\r\n", code, reason);
    sprintf(headers, "%sServer: Vino\r\n", headers);
    sprintf(headers, "%sContent-type: %s\r\n", headers, content_type);
    sprintf(headers, "%sConnection: %s\r\n", headers, conn);
    sprintf(headers, "%sContent-length: %d\r\n", headers, (int)content_length);
    sprintf(headers, "%s\r\n", headers);
}

void vn_build_resp_404_body(char *body, const char *uri) {
    sprintf(body, "<html><head>");
    sprintf(body, "%s<title>404 Not Found</title>", body);
    sprintf(body, "%s</head><body>", body);
    sprintf(body, "%s<h1>Not Found</h1>", body);
    sprintf(body, "%s<p>The requested URL %s was not found on this server.</p>", body, uri);
    sprintf(body, "%s<hr>", body);
    sprintf(body, "%s<address>Vino Server</address>", body);
    sprintf(body, "%s</body></html>", body);
}

void vn_build_resp_403_body(char *body, const char *uri) {
    sprintf(body, "<html><head>");
    sprintf(body, "%s<title>403 Forbidden</title>", body);
    sprintf(body, "%s</head><body>", body);
    sprintf(body, "%s<h1>Forbidden</h1>", body);
    sprintf(body, "%s<p>You don't have permission to access %s on this server.</p>", body, uri);
    sprintf(body, "%s<hr>", body);
    sprintf(body, "%s<address>Vino Server</address>", body);
    sprintf(body, "%s</body></html>", body);    
}

const char *vn_status_message(int code) {
    const char *msg = NULL;
    switch (code) {
        case 200:
            msg = "OK";                    break;
        case 400:
            msg = "Bad Request";           break;
        case 401:
            msg = "Unauthorized";          break;
        case 403:
            msg = "Forbidden";             break;
        case 404:
            msg = "Not Found";             break;
        case 500:
            msg = "Internal Server Error"; break;
        case 503:
            msg = "Service Unavailable";   break;
        default:
            msg = "OK";
    }
    return msg;
}

static void vn_print_http_request(vn_http_request *hr) {
    int i;
    char name[VN_MAX_HTTP_HEADER_NAME], value[VN_MAX_HTTP_HEADER_VALUE];
    vn_linked_list_node *name_node, *value_node;
    vn_str *name_str, *value_str;

    if (vn_get_string(&hr->method, value, VN_MAX_HTTP_HEADER_VALUE) < 0) {
        err_sys("[print_http_request] vn_get_string [method] error");
    }
    printf("\nMethod: %s\n", value);

    if (vn_get_string(&hr->uri, value, VN_MAX_HTTP_HEADER_VALUE) < 0) {
        err_sys("[print_http_request] vn_get_string [uri] error");
    }
    printf("URI: %s\n", value);

    if (vn_get_string(&hr->proto, value, VN_MAX_HTTP_HEADER_VALUE) < 0) {
        err_sys("[print_http_request] vn_get_string [proto] error");
    }
    printf("Proto: %s\n", value);

    if (vn_get_string(&hr->query_string, value, VN_MAX_HTTP_HEADER_VALUE) < 0) {
        err_sys("[print_http_request] vn_get_string [query_string] error");
    }
    printf("Query-String: %s\n", value);

    name_node = hr->header_name_list.head;
    value_node = hr->header_value_list.head;
    while (name_node && value_node) {
        name_str = (vn_str *) name_node->data;
        value_str = (vn_str *) value_node->data;
        if (vn_get_string(name_str, name, VN_MAX_HTTP_HEADER_NAME) < 0 ||
            vn_get_string(value_str, value, VN_MAX_HTTP_HEADER_VALUE) < 0) {
            err_sys("[print_http_request] vn_get_string [header] error");
        }
        printf("| %s | %s |\n", name, value);  
        name_node = name_node->next;
        value_node = value_node->next;      
    }

    // if (hr->body.len > 0 && vn_get_string(&hr->body, value, VN_MAX_HTTP_HEADER_VALUE) < 0) {
    //     err_sys("[print_http_request] vn_get_string [body] error");
    // }
    // printf("\nBody:\n%s", value);

}
