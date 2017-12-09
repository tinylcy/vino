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
static int   keep_alive_g = VN_CONN_KEEP_ALIVE;    /* Global Keep-Alive flag, which is on by default */

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
            "    -p|--port <port>    Specify port for Vino. 8080 is the default port.\n"
            "    -h|--help           This information.\n"
            "    -V|--version        Display program version.\n"
            "    -k  <on|off>        Use HTTP Keep-Alive featue or not. Keep-Alive is on by default.\n",
            program
    );
}

static void vn_parse_options(int argc, char *argv[]) {
    int opt;
    int options_index;
    size_t optarg_len;

    if (1 == argc) {
        return;
    }

    while ((opt = getopt_long(argc, argv, "Vp:k:?h", long_options, &options_index)) != EOF) {
        switch (opt) {
            case  0 : break;
            case 'p': port = optarg; break;
            case 'V': printf(VINO_VERSION"\n"); exit(0);
            case 'k': 
                optarg_len = strlen(optarg);
                if (optarg_len != 2 && optarg_len != 3) {
                    fprintf(stderr, "Error in option -%c: Illegal argument %s.\n", opt, optarg);
                    exit(-1);
                }
                if (!strcasecmp(optarg, "on")) {
                    keep_alive_g = VN_CONN_KEEP_ALIVE;
                } else if (!strcasecmp(optarg, "off")) {
                    keep_alive_g = VN_CONN_CLOSE;
                } else {
                    fprintf(stderr, "Error in option -%c: Illegal argument %s.\n", opt, optarg);
                    exit(-1);
                }
                break;
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

static void vn_print_http_request(vn_http_request_t *hr);

int main(int argc, char *argv[]) {
    int rv, epfd, listenfd;
    int i, nready, fd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; 
    struct epoll_event ep_event;
    vn_http_connection_t *http_conn, *conn;
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

    if ((http_conn = (vn_http_connection_t *) malloc(sizeof(vn_http_connection_t))) == NULL) {
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
            conn = (vn_http_connection_t *) events[i].data.ptr;
            fd = conn->fd;

            if (!((events[i].events & EPOLLIN) || (events[i].events & EPOLLOUT))) {
                vn_log_warn("An error has occured on file descriptor [%d], or the socket is not ready for reading or writing.", fd);
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

#ifdef DEBUG
                    DEBUG_PRINT("Accept new incoming connection, [fd = %d]", connfd);
#endif
                    
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

                    vn_http_connection_t *new_conn;
                    struct epoll_event new_ep_ev;

                    if ((new_conn = (vn_http_connection_t *) malloc(sizeof(vn_http_connection_t))) == NULL) {
                        err_sys("[main] malloc vn_http_connection error");
                    }

                    vn_init_http_connection(new_conn, connfd, epfd);
                    new_ep_ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
                    new_ep_ev.data.ptr = (void *) new_conn;
                    vn_epoll_add(epfd, connfd, &new_ep_ev);

                    vn_event_add_timer(new_conn, VN_DEFAULT_TIMEOUT);

#ifdef DEBUG
                    DEBUG_PRINT("A new connection has been add to timer, [fd = %d]", connfd);
#endif
                    
                }
            /* End of fd == listenfd */

            } else {
                vn_handle_http_connection(events[i].events, (vn_http_connection_t *) events[i].data.ptr);
            }
        }
    }

    return 0;
}

void vn_handle_http_connection(uint32_t events, vn_http_connection_t *conn) {
    if (events & EPOLLIN) {
        vn_handle_read_event(conn);
    } else if(events & EPOLLOUT) {
        vn_handle_write_event(conn);
    }
}

void vn_handle_read_event(vn_http_connection_t *conn) {
    int nread, buf_len;
    int rv, req_line_completed, req_headers_completed;

    while (VN_KEEP_READING) {
        nread = read(conn->fd, conn->req_buf_ptr, conn->remain_size);
    
#ifdef DEBUG
        DEBUG_PRINT("Read HTTP request message from connection [fd = %d], nread = %d", conn->fd, nread);
#endif

        /* End of file, the remote has closed the connection. */
        if (0 == nread) {
            break;
        }

        /* If errno == EAGAIN, that means we have read all data. */
        if (nread < 0) {
            if (errno == EINTR) {
                continue;
            } if (errno != EAGAIN && errno != EWOULDBLOCK) {
                err_sys("[vn_handle_read_event] rio_readn error");
            }
            break;
        }

        conn->req_buf_ptr += nread;
        conn->request.last += nread;     /* Update the last character parser can read */
        conn->remain_size -= nread;

        rv = vn_http_parse_request_line(conn, conn->req_buf);
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
#ifdef DEBUG
            DEBUG_PRINT("The HTTP request line has been successfully parsed, [fd = %d]", conn->fd);
#endif
        }

        while (VN_KEEP_PARSING) {
            rv = vn_http_parse_header_line(conn, conn->req_buf);
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
#ifdef DEBUG
            DEBUG_PRINT("The HTTP request headers have been successfully parsed, [fd = %d]", conn->fd);
#endif
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

void vn_handle_write_event(vn_http_connection_t *conn) {
    ssize_t nwritten;
    int connection = VN_CONN_KEEP_ALIVE;

    if (conn->resp_headers == NULL || conn->resp_body == NULL) {
        return;
    }

    /* Send HTTP response headers (include response line) */
    while (VN_KEEP_WRITING) {
        if ((nwritten = write(conn->fd, conn->resp_headers_ptr, conn->resp_headers_left)) < 0) {
#ifdef DEBUG
            DEBUG_PRINT("Write HTTP response headers to the remote, nwritten = %ld, [fd = %d]", nwritten, conn->fd);
#endif
            if (errno == EINTR) {
                continue;
            } if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            } else {
                err_sys("[vn_handle_write_event] write HTTP response headers error");
            }
        }

#ifdef DEBUG
            DEBUG_PRINT("Write HTTP response headers to the remote, nwritten = %ld, [fd = %d]", nwritten, conn->fd);
#endif

        if (nwritten == 0) {
            break;
        }

        conn->resp_headers_ptr += nwritten;
        conn->resp_headers_left -= nwritten;
    }

    /* Send HTTP response body */
    while (VN_KEEP_WRITING) {
        if ((nwritten = write(conn->fd, conn->resp_body_ptr, conn->resp_body_left)) < 0) {
#ifdef DEBUG
            DEBUG_PRINT("Write HTTP response body to the remote, nwritten = %ld, [fd = %d]", nwritten, conn->fd);
#endif 
            if (errno == EINTR) {
                continue;
            } if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            } else {
                err_sys("[vn_handle_write_event] write HTTP response body error");
            }
        }

#ifdef DEBUG
            DEBUG_PRINT("Write HTTP response body to the remote, nwritten = %ld, [fd = %d]", nwritten, conn->fd);
#endif

        if (nwritten == 0) {
            if (munmap((void *) conn->resp_body, conn->resp_file_size) < 0) {
                err_sys("[vn_handle_write_event] munmap error");
            }
            break;
        }

        conn->resp_body_ptr += nwritten;
        conn->resp_body_left -= nwritten;
    }

    /* 
     * If persistent connection flag (Connection: keep-alive) is set,
     * several connections will share the same buffer, therefore after finishing
     * one HTTP request the buffer should be reset.
     */
    if (conn->keep_alive == VN_CONN_KEEP_ALIVE) {
        vn_event_add_timer(conn, VN_DEFAULT_TIMEOUT);     /* Restart timer */
        vn_reset_http_connection(conn);                   /* Reset connection, the RESET operations only reset buffer area */
    } else if (conn->resp_body_left == 0){
        vn_close_http_connection((void *) conn);         
    }
    
}

void vn_handle_get_connection(vn_http_connection_t *conn) {
    vn_http_request_t *req;
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
    unsigned int file_size;
    char mime_type[VN_FILETYPE_SIZE];
    vn_str *conn_str;
    ssize_t nwritten;
    size_t headers_len;
    
    if (vn_check_file_exist(filepath) < 0) {
        vn_build_resp_404_body(body, uri);
        vn_build_resp_headers(headers, 404, "Not Found", "text/html", strlen(body), VN_CONN_CLOSE);
        // TODO: using vn_handle_write_event
        rio_writen(conn->fd, (void *) headers, strlen(headers));
        rio_writen(conn->fd, (void *) body, strlen(body));
        vn_close_http_connection((void *) conn);
        return;
    } 
    
    if (vn_check_read_permission(filepath) < 0) {
        vn_build_resp_403_body(body, uri);
        vn_build_resp_headers(headers, 403, "Forbidden", "text/html", strlen(body), VN_CONN_CLOSE);
        // TODO: using vn_handle_write_event
        rio_writen(conn->fd, (void *) headers, strlen(headers));
        rio_writen(conn->fd, (void *) body, strlen(body));
        vn_close_http_connection((void *) conn);
        return;
    }

    if ((srcfd = open(filepath, O_RDONLY, 0)) < 0) {
        err_sys("[vn_handle_get_connection] open error");
    }
    file_size = vn_get_filesize(filepath);
    /* Map the target file into memory */
    if ((srcp = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, srcfd, 0)) == MAP_FAILED) {
        vn_log_warn("Call mmap error, filepath = %s\n", filepath);
        err_sys("[vn_handle_get_connection] mmap error");
    }
    // vn_log_info("Mmap file success, filepath = %s", filepath);
    if (close(srcfd) < 0) {
        err_sys("[vn_handle_get_connection] close srcfd error");
    }

    /* Build response header by Connection header */
    vn_get_mime_type(filepath, mime_type);
    conn_str = vn_get_http_header(req, "Connection");
    if (NULL != conn_str && (!vn_str_cmp(conn_str, "keep-alive") || !vn_str_cmp(conn_str, "Keep-Alive"))) {
        vn_build_resp_headers(headers, 200, "OK", mime_type, file_size, VN_CONN_KEEP_ALIVE);
        conn->keep_alive = (keep_alive_g == VN_CONN_CLOSE) ? VN_CONN_CLOSE : VN_CONN_KEEP_ALIVE;
    } else {
        vn_build_resp_headers(headers, 200, "OK", mime_type, file_size, VN_CONN_CLOSE);
        conn->keep_alive = VN_CONN_CLOSE;
    }

    /* Allocate HTTP response buffer */
    headers_len = strlen(headers);
    if ((conn->resp_headers = (char *) vn_palloc(conn->pool, sizeof(char) * headers_len)) == NULL) {
        err_sys("[vn_handle_get_connection] vn_palloc response headers buffer error");
    }
    strncpy(conn->resp_headers, headers, headers_len);
    conn->resp_headers_ptr = conn->resp_headers;
    conn->resp_headers_left = headers_len;
    conn->resp_body = (char *) srcp;
    conn->resp_body_ptr = conn->resp_body;
    conn->resp_body_left = conn->resp_file_size = file_size;
    
    /* Send HTTP response */
    vn_handle_write_event(conn);

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

static void vn_print_http_request(vn_http_request_t *hr) {
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
