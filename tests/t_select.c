#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/select.h>

static void usage_error(const char *prog) {
	fprintf(stderr, "Usage: %s {timeout|-} fd-num[rw]...\n", prog);
	fprintf(stderr, "    - means infinite timeout;\n");
	fprintf(stderr, "    r = monitor for read\n");
	fprintf(stderr, "    w = monitor for write\n\n");
	fprintf(stderr, "    e.g.: %s - 0rw 1w\n", prog);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
	fd_set readfds, writefds;
	int ready, nfds, fd, num_read, j;
	struct timeval timeout;
	struct timeval *pto;
	char buf[10];

	if(argc < 2 || strcmp(argv[1], "--help") == 0) {
		usage_error(argv[0]);
	}

	if(strcmp(argv[1], "-") == 0) {
		pto = NULL;
	} else {
		pto = &timeout;
		timeout.tv_sec = atoi(argv[1]);
		timeout.tv_usec = 0;
	}

	nfds = 0;
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);

	for(j = 2; j < argc; j++) {
		num_read = sscanf(argv[j], "%d%2[rw]", &fd, buf);
		if(num_read != 2) {
			usage_error(argv[0]);
		}
		if(fd >= FD_SETSIZE) {
			perror("file descriptor exceeds limit");
			exit(EXIT_FAILURE);
		}
		if(fd > nfds) {
			nfds = fd + 1;
		}
		if(strchr(buf, 'r') != NULL) {
			FD_SET(fd, &readfds);
		}
		if(strchr(buf, 'w') != NULL) {
			FD_SET(fd, &writefds);
		}
	}

	ready = select(nfds, &readfds, &writefds, NULL, pto);

	if(ready == -1) {
		perror("select");
		exit(1);
	}

	printf("ready: %d\n", ready);
	for(fd = 0; fd < nfds; fd++) {
		printf("%d: %s%s\n", fd, FD_ISSET(fd, &readfds) ? "r" : "", FD_ISSET(fd, &writefds) ? "w" : "");
	}

	if(pto != NULL) {
		printf("timeout after select(): %ld.%03ld\n", (long)timeout.tv_sec, (long)timeout.tv_usec / 10000);
	}

	exit(EXIT_SUCCESS);
}
