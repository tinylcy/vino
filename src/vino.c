/*
 *  Copyright (C) Chenyang Li
 *  Copyright (C) Vino
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
#include "socketlib.h"
#include "vn_request.h"
#include "vn_epoll.h"
#include "vn_event_timer.h"
#include "vn_logger.h"
#include "util.h"
#include "error.h"

static char *port = VN_PORT;

static const struct option long_options[] = {
    { "port", required_argument, NULL, 'p' },
    { "help", no_argument, NULL, '?' },
    { "version", no_argument, NULL, 'V' },
    { NULL, 0, NULL, 0 }
};

static void vn_usage(char *program) {
    fprintf(stderr,
            "%s [option]...\n"
            " -p|--port <port>       Specify port for vino. Default 8080.\n"
            " -?|-h|--help           This information.\n"
            " -V|--version           Display program version.\n",
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

int main(int argc, char *argv[]) {
    int rv, epfd, listenfd;
    int i, nready, fd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; 
    struct epoll_event ep_event;
    vn_http_event *http_event;
    vn_msec_t time;

    vn_parse_options(argc, argv);

    /* 
     * Install signal handler for SIGPIPE.
     * When a HTTP response contains header `Connection: close`,
     * the browser will close the socket after receive data, which
     * will create a SIGPIPE signal to the process.
     */
    vn_signal(SIGPIPE, SIG_IGN);

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

    if ((http_event = (vn_http_event *) malloc(sizeof(vn_http_event))) == NULL) {
        err_sys("[main] malloc vn_http_event error");
    }
    vn_init_http_event(http_event, listenfd, epfd);

    ep_event.events = EPOLLIN | EPOLLET;
    ep_event.data.ptr = (void *) http_event;
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

        vn_time_update();
        vn_event_expire_timers();

        /* Deal with returned list of events */
        for (i = 0; i < nready; i++) {
            vn_http_event *event = (vn_http_event *) events[i].data.ptr;
            fd = event->fd;

            if (listenfd == fd) {
                while (VN_ACCEPT) {
                    clientlen = sizeof(struct sockaddr_storage);                
                    connfd = accept(listenfd, (struct sockaddr *) &clientaddr, &clientlen);
                    if (connfd < 0) {
                        if (errno == EAGAIN) {
                            break;  /* All connections have been processed */
                        } else {
                            err_sys("[main] accept error");
                        }
                    }

                    if ((rv = make_socket_non_blocking(connfd)) != 0) {
                        err_sys("[main] make_socket_non_blocking error");
                    }

                    vn_http_event *new_http_ev;
                    struct epoll_event new_ep_ev;

                    if ((new_http_ev = (vn_http_event *) malloc(sizeof(vn_http_event))) == NULL) {
                        err_sys("[main] malloc vn_http_event error");
                    }

                    vn_init_http_event(new_http_ev, connfd, epfd);
                    new_ep_ev.events = EPOLLIN | EPOLLET;
                    new_ep_ev.data.ptr = (void *) new_http_ev;
                    vn_epoll_add(epfd, connfd, &new_ep_ev);

                    vn_event_add_timer(new_http_ev, VN_DEFAULT_TIMEOUT);
                    
                }
            /* End of fd == listenfd */

            } else {
                vn_handle_http_event(events[i].data.ptr);
            }
        }
    }


    return 0;
}

void vn_init_http_event(vn_http_event *event, int fd, int epfd) {
    event->fd = fd;
    event->epfd = epfd;
    memset(event->buf, '\0', VN_BUFSIZE);
    event->bufptr = event->buf;
    event->remain_size = VN_BUFSIZE;
    vn_init_http_request(&event->hr);
    event->handler = vn_close_http_event;
    event->pq_node = NULL;
}

void vn_handle_http_event(vn_http_event *event) {
    int nread, buf_len;
    int rv;
    vn_http_request *hr;

    while ((nread = rio_readn(event->fd, event->bufptr, event->remain_size)) > 0) {
        event->bufptr += nread;
        event->remain_size -= nread;
    }

    if (0 == nread) {
        // TODO: logger
    }

    if (nread < 0) {
        if (errno == EAGAIN) {
            // break;
        } else {
            err_sys("[vn_handle_http_event] rio_readn error");
        }
    }

    /*
     * If a HTTP request message has not been buffered completely, do nothing.
     * Otherwise, parse it.
     */
    if ((buf_len = vn_http_get_request_len(event->buf, VN_BUFSIZE)) < 0) {
        err_sys("[vn_serve_http_event] vn_http_get_request_len error");
    } else if (buf_len > 0) {
        if (vn_parse_http_request(event->buf, buf_len, &event->hr) < 0) {
            err_sys("[vn_serve_http_event] vn_parse_http_request error");
        }
    } else {
        // TODO: logger
        return; 
    }

    hr = &event->hr;

    // vn_print_http_request(&event->hr);

    if (!vn_str_cmp(&hr->method, "GET")) {
        vn_handle_get_event(event);
    } else if (!vn_str_cmp(&hr->method, "POST")) {
        // TODO
    }

}

void vn_handle_get_event(vn_http_event *event) {
    vn_http_request *hr;
    char uri[VN_MAX_HTTP_HEADER_VALUE], filepath[VN_MAX_HTTP_HEADER_VALUE];

    /* 
     * Remove the corresponding node in priority queue. 
     * This is because the event will be dealt with soon.
     */
    vn_pq_delete_node(event->pq_node);

    hr = &event->hr;
    if (vn_get_string(&hr->uri, uri, VN_MAX_HTTP_HEADER_VALUE) == -1) {
        err_sys("[vn_handle_get_event] vn_get_string error");
    }
    if (strlen(uri) == 1 && !strncmp(uri, "/", 1)) {
        strncpy(uri, VN_DEFAULT_PAGE, strlen(VN_DEFAULT_PAGE));
    }

    /* Append default static resource path before HTTP request's uri */
    memset(filepath, '\0', VN_MAX_HTTP_HEADER_VALUE);
    if (strcat(filepath, VN_PARENT_DIR) == NULL) {
        err_sys("[vn_handle_get_event] strcat [VN_PARENT_DIR] error");
    }
    if (strcat(filepath, VN_DEFAULT_STATIC_RES_DIR) == NULL) {
        err_sys("[vn_handle_get_event] strcat [DEFAULT_STATIC_RES_DIR] error");
    }
    if (strncat(filepath, uri, strlen(uri)) == NULL) {
        err_sys("[vn_handle_get_event] strcat [uri] error");
    }

    char headers[VN_HEADERS_SIZE], body[VN_BODY_SIZE];
    int srcfd;
    void *srcp;
    unsigned int filesize;
    char filetype[VN_FILETYPE_SIZE];
    vn_str *connection;
    short conn_flag;
    
    // TODO: nwrite = rio_writen(fd, buf, size); Check if `nwrite` == `size`.
    if (vn_check_file_exist(filepath) < 0) {
        vn_build_resp_404_body(body, uri);
        vn_build_resp_headers(headers, 404, "Not Found", "text/html", strlen(body), VN_CONN_CLOSE);
        rio_writen(event->fd, (void *) headers, strlen(headers));
        rio_writen(event->fd, (void *) body, strlen(body));
        vn_close_http_event((void *) event);
        return;
    } 
        // TODO: Check permission
    if ((srcfd = open(filepath, O_RDONLY, 0)) < 0) {
        err_sys("[vn_handle_get_event] open error");
    }
    filesize = vn_get_filesize(filepath);
    /* Map the target file into memory */
    if ((srcp = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0)) == MAP_FAILED) {
        err_sys("[vn_handle_get_event] mmap error");
    }
    if (close(srcfd) < 0) {
        err_sys("[vn_handle_get_event] close srcfd error");
    }

    /* Build response header by Connection header */
    vn_get_filetype(filepath, filetype);
    connection = vn_get_http_header(hr, "Connection");
    if (NULL != connection && !vn_str_cmp(connection, "keep-alive")) {
        vn_build_resp_headers(headers, 200, "OK", filetype, filesize, VN_CONN_KEEP_ALIVE);
        conn_flag = VN_CONN_KEEP_ALIVE;
    } else {
        vn_build_resp_headers(headers, 200, "OK", filetype, filesize, VN_CONN_CLOSE);
        conn_flag = VN_CONN_CLOSE;
    }

    /* Send response */
    rio_writen(event->fd, headers, strlen(headers));
    rio_writen(event->fd, srcp, filesize);
    if (munmap(srcp, filesize) < 0) {
        err_sys("[vn_handle_get_event] munmap error");
    }

    /* 
     * If persistent connection flag (Connection: keep-alive) is set,
     * several connections will share the same buffer, therefore the buffer
     * should be reset before accepting the next connection.
     */
    if (VN_CONN_KEEP_ALIVE == conn_flag) {
        memset(event->buf, '\0', VN_BUFSIZE);
        event->bufptr = event->buf;
        event->remain_size = VN_BUFSIZE;
        vn_event_add_timer(event, VN_DEFAULT_TIMEOUT);
    } else {
        vn_close_http_event((void *) event);
    }

}

void vn_close_http_event(void *event) {
    vn_http_event *ev = (vn_http_event *) event;
    if (close(ev->fd) < 0) {
        err_sys("[vn_close_http_event] close error");
    }
    free(ev);
}

void vn_build_resp_headers(char *headers, int code, const char *reason, const char *content_type, 
                            unsigned int content_length, short keep_alive) {
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