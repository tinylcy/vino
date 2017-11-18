/*
 *  Copyright (C) Chenyang Li
 *  Copyright (C) vino
 */
#include "vino.h"
#include "socketlib.h"
#include "util.h"
#include "error.h"

#define PORT "8080"

int main(int argc, char *argv[]) {
    int rv, listenfd;

    if ((listenfd = open_listenfd(PORT)) != 0) {
        err_sys("[main] open_listenfd error");
    }

    if((rv = make_socket_non_blocking(listenfd)) != 0) {
        err_sys("[main] make_socket_non_blocking error");
    }

    
    return 0;
}
