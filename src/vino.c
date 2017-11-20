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
#include "util.h"
#include "error.h"

#define PORT        "8080"
#define VN_RUNNING  1
#define VN_ACCEPT   1

int main(int argc, char *argv[]) {
    int rv, epfd, listenfd;
    int i, nready, fd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; 
    struct epoll_event ep_event;
    vn_http_event *http_event;

    /* 
     * Install signal handler for SIGPIPE.
     * When a HTTP response contains header `Connection: close`,
     * the browser will close the socket after receive data, which
     * will create a SIGPIPE signal to the process.
     */
    vn_signal(SIGPIPE, SIG_IGN);

    if ((listenfd = open_listenfd(PORT)) < 0) {
        err_sys("[main] open_listenfd error");
    }

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

    while (VN_RUNNING) {
        nready = vn_epoll_wait(epfd, events, VN_MAXEVENTS, -1);
        if (nready < 0) {
            if (errno == EINTR) {
                continue;   /* Restart if interrupted by signal */
            } else {
                err_sys("[main] vn_epoll_wait error");
            }
        }

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
}

void vn_handle_http_event(vn_http_event *event) {
    int nread, buf_len;
    int rv;
    rio_t rio;
    vn_http_request *hr;

    rio_readinitb(&rio, event->fd);

    while ((nread = rio_readn(event->fd, event->bufptr, event->remain_size)) > 0) {
        event->bufptr += nread;
        event->remain_size -= nread;
    }

    if (nread < 0) {
        if (errno != EAGAIN) {
            err_sys("[vn_handle_http_event] rio_readn error");
        } 
    }

    /*
     * If a HTTP request message has not been completely, do nothing.
     * Otherwise, parse it.
     */
    if ((buf_len = vn_http_get_request_len(event->buf, VN_BUFSIZE)) < 0) {
        err_sys("[vn_serve_http_event] vn_http_get_request_len error");
    } else if (buf_len > 0) {
        if (vn_parse_http_request(event->buf, buf_len, &event->hr) < 0) {
            err_sys("[vn_serve_http_event] vn_parse_http_request error");
        }
    } else {
        printf("Haven't buffer completely\n");
        return; 
    }

    hr = &event->hr;

    vn_print_http_request(&event->hr);

    if (!vn_str_cmp(&hr->method, "GET")) {
        vn_handle_get_event(event);
    } else if (!vn_str_cmp(&hr->method, "POST")) {

    }

}

void vn_handle_get_event(vn_http_event *event) {
    vn_http_request *hr;
    char uri[VN_MAX_HTTP_HEADER_VALUE], filepath[VN_MAX_HTTP_HEADER_VALUE];

    hr = &event->hr;
    if (vn_get_string(&hr->uri, uri, VN_MAX_HTTP_HEADER_VALUE) == -1) {
        err_sys("[vn_handle_get_event] vn_get_string error");
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

    printf("filepath = %s\n", filepath);

    /* Check if a file exist */
    char headers[VN_HEADERS_SIZE], body[VN_BODY_SIZE];
    int srcfd;
    void *srcp;
    unsigned int filesize;
    char filetype[VN_FILETYPE_SIZE];
    
    // TODO: nwrite = rio_writen(fd, buf, size); Check if `nwrite` == `size`.
    if (vn_check_file_exist(filepath) < 0) {
        vn_build_resp_error_body(body);
        vn_build_resp_headers(headers, 404, "Not Found", "text/html", strlen(body));
        rio_writen(event->fd, (void *) headers, strlen(headers));
        rio_writen(event->fd, (void *) body, strlen(body));
    } else {
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
        vn_get_filetype(filepath, filetype);
        vn_build_resp_headers(headers, 200, "OK", filetype, filesize);
        rio_writen(event->fd, headers, strlen(headers));
        rio_writen(event->fd, srcp, filesize);
        if (munmap(srcp, filesize) < 0) {
            err_sys("[vn_handle_get_event] munmap error");
        }
    }

}

void vn_build_resp_headers(char *headers, int code, const char *reason, const char *content_type, 
                            unsigned int content_length) {
    if (NULL == reason) {
        reason = vn_status_message(code);
    }
    sprintf(headers, "HTTP/1.1 %d %s\r\n", code, reason);
    sprintf(headers, "%sServer: Vino\r\n", headers);
    sprintf(headers, "%sContent-type: %s\r\n", headers, content_type);
    sprintf(headers, "%sConnection: close\r\n", headers);
    sprintf(headers, "%sContent-length: %d\r\n", headers, (int)content_length);
    sprintf(headers, "%s\r\n", headers);
}

void vn_build_resp_error_body(char *body) {
    sprintf(body, "<html><title>Vino Error</title>");
    sprintf(body, "%s<body bgcolor=\"ffffff\">\n", body);
    sprintf(body, "%sVino Error.", body);
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