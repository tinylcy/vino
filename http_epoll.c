/*
 * Copyright (C) Chenyang Li
 * Copyright (C) tinyhttpd
 */

#include <stdio.h>
#include <stdlib.h>
#include "http_epoll.h"

int http_epoll_create(int flag) {
	int epfd = -1;
	epfd = epoll_create1(flag);
	if(epfd == -1) {
		perror("epoll_create1");
		return -1;
	}

	evlist = (struct epoll_event*)malloc(MAXEVENTS * sizeof(struct epoll_event));
	if(evlist == NULL) {
		perror("malloc");
		return -1;
	}

	return epfd;
}

int http_epoll_add(int epfd, int fd, struct epoll_event *event) {
	int s = -1;
	s = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, event);
	if(s == -1) {
		perror("http_epoll_add");
		return -1;
	}

	return 0;
}

int http_epoll_mod(int epfd, int fd, struct epoll_event *event) {
	int s = -1;
	s = epoll_ctl(epfd, EPOLL_CTL_MOD, fd, event);
	if(s == -1) {
		perror("http_epoll_mod");
		return -1;
	}

	return 0;
}

int http_epoll_del(int epfd, int fd, struct epoll_event *event) {
	int s = -1;
	s = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, event);
	if(s == -1) {
		perror("http_epoll_del");
		return -1;
	}

	return 0;
}

int http_epoll_wait(int epfd, struct epoll_event *evlist, int maxevents, int timeout) {
	int n = epoll_wait(epfd, evlist, maxevents, timeout);
	if(n == -1) {
		perror("http_epoll_wait");
		return -1;
	}

	return n;
}
