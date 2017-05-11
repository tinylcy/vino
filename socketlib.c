/*
 * Copyright (C) Chenyang Li
 * Copyright (C) tinyhttpd
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <strings.h>
#include "socketlib.h"

int make_server_socket(int port) {
	return make_server_socket_back(port, BACKLOG);
}

int make_server_socket_back(int port, int backlog) {
	int sock_id;
	struct sockaddr_in servaddr;

	// create socket.
	sock_id = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_id == -1) {
		fprintf(stderr, "socket");
		return -1;
	}

	// prepare the sockaddr_in structure.
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	// servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servaddr.sin_port = htons(port);

	// bind.
	if (bind(sock_id, (struct sockaddr *)&servaddr , sizeof(servaddr)) < 0) {
		fprintf(stderr, "bind");
		return -1;
	}

	// start listening to the socket.
	if (listen(sock_id, backlog) != 0) {
		fprintf(stderr, "listen");
		return -1;
	}

	return sock_id;
}

int connect_to_server(char *host, int portnum) {
	int sock_id;
	struct sockaddr_in servaddr;
	struct hostent *hp;

	sock_id = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_id == -1) {
		return -1;
	}

	bzero(&servaddr, sizeof(servaddr));
	hp = gethostbyname(host);
	if (hp == NULL) {
		return -1;
	}
	bcopy(hp->h_addr, (struct sockaddr*)&servaddr.sin_addr, hp->h_length);
	servaddr.sin_port = htons(portnum);
	servaddr.sin_family = AF_INET;

	if (connect(sock_id, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0) {
		return -1;
	}

	return sock_id;
}

int make_socket_non_blocking(int sfd) {
	int flags, s;
	flags = fcntl(sfd, F_GETFL, 0);
	if (flags == -1) {
		perror("fcntl");
		return -1;
	}

	flags |= O_NONBLOCK;
	s = fcntl(sfd, F_SETFL, flags);
	if (s == -1) {
		perror("fcntl");
		return -1;
	}

	return 0;
}
