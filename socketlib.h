/*
 *  Copyright (C) Chenyang Li
 *  Copyright (C) vino
 */

/* The backlog argument defines the maximum length to which 
the queue of pending connections for sockfd may grow */
#define BACKLOG 1024

/* Create a connection with server */
int open_clientfd(const char *hostname, const char *port);

/* Create a listen fd and be ready for accepting connections */
int open_listenfd(const char *port);