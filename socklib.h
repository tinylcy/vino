/*
 * Copyright (C) Chenyang Li
 * Copyright (C) tinyhttpd
 */

#define HOSTLEN 256
#define BACKLOG 1

int make_server_socket(int);

int make_server_socket_back(int, int);

int connect_to_server(char*, int);

int make_socket_non_blocking(int);
