/*
 *  Copyright (C) Chenyang Li
 *  Copyright (C) vino
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include "vino.h"
#include "rio.h"
#include "socketlib.h"
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
             err_sys("[vn_server_http_event] rio_readn error");
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
    }

    hr = &event->hr;

    vn_print_http_request(&event->hr);

    if (!vn_str_cmp(&hr->method, "GET")) {
        vn_handle_get_event(event);
    } else if (!vn_str_cmp(&hr->method, "POST")) {

    }

}

void vn_handle_get_event(vn_http_event *event) {
    printf("In handle_get_event....\n");
}

void vn_send_resp_headers(vn_http_event *event, int code, unsigned long content_length) {

}

void vn_send_resp_error(vn_http_event *event, int code, const char *reason) {

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