/*
 * socklib.c
 *
 * socklib.c 包含了在创建客户端/服务端程序时常用到的函数。
 * 主要包含：
 * 
 * int make_server_socket(int portnum);
 *
 * int make_server_sorket_q(int portnum, int backlog);
 *
 * int connect_to_server(char *hostname, int portnum);
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <strings.h>

#define HOSTLEN 256
#define BACKLOG 1

int make_server_socket_q(int, int);

int make_server_socket(int portnum) {
	return make_server_socket_q(portnum, BACKLOG);
}

int make_server_socket_q(int portnum, int backlog) {
	struct sockaddr_in servaddr;
	struct hostent *hp;
	char hostname[BUFSIZ];
	int sock_id;

	sock_id = socket(PF_INET, SOCK_STREAM, 0);
	if(sock_id == -1) {
		return -1;
	}

	/*
	 * 创建地址并将地址绑定到socket
	 */
	bzero((void*)&servaddr, sizeof(servaddr));
	gethostname(hostname, HOSTLEN);
	hp = gethostbyname(hostname);

	bcopy((void*)hp->h_addr, (void*)&servaddr.sin_addr, hp->h_length);
	servaddr.sin_port = htons(portnum);
	servaddr.sin_family = AF_INET;
	if(bind(sock_id, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
		return -1;
	}

	/*
	 * 开启对socket的监听
	 */
	if(listen(sock_id, backlog) != 0) {
		return -1;
	}

	return sock_id;
}

int connect_to_server(char *host, int portnum) {
	int sock_id;
	struct sockaddr_in servaddr;
	struct hostent *hp;

	sock_id = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_id == -1) {
		return -1;
	}

	bzero(&servaddr, sizeof(servaddr));
	hp = gethostbyname(host);
	if(hp == NULL) {
		return -1;
	}
	bcopy(hp->h_addr, (struct sockaddr*)&servaddr.sin_addr, hp->h_length);
	servaddr.sin_port = htons(portnum);
	servaddr.sin_family = AF_INET;

	if(connect(sock_id, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0) {
		return -1;
	}

	return sock_id;
}
