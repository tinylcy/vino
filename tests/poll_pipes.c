#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <poll.h>

int main(int argc, char *argv[]) {
	int num_pipes, j, ready, rand_pipe, num_writes;
	int (*pfds)[2];
	struct pollfd *poll_fd;

	if(argc < 2 || strcmp(argv[1], "--help") == 0) {
		fprintf(stderr, "Usage: %s num-pipes [num-writes]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	num_pipes = atoi(argv[1]);

	pfds = calloc(num_pipes, sizeof(int[2]));
	if(pfds == NULL) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	poll_fd = calloc(num_pipes, sizeof(struct pollfd));
	if(poll_fd == NULL) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	for(j = 0; j < num_pipes; j++) {
		if(pipe(pfds[j]) == -1) {
			perror("pipe");
			exit(EXIT_FAILURE);
		}
	}

	num_writes = (argc > 2) ? atoi(argv[2]) : 1;
	srandom((int)time(NULL));
	for(j = 0; j < num_writes; j++) {
		rand_pipe = random() % num_pipes;
		printf("Writing to fd: %3d (read fd: %3d)\n", pfds[rand_pipe][1], pfds[rand_pipe][0]);
		if(write(pfds[rand_pipe][1], "a", 1) == -1) {
			perror("write");
			exit(EXIT_FAILURE);
		}
	}

	for(j = 0; j < num_pipes; j++) {
		poll_fd[j].fd = pfds[j][0];
		poll_fd[j].events = POLLIN;
	}

	ready = poll(poll_fd, num_pipes, -1);
	if(ready == -1) {
		perror("poll");
		exit(EXIT_FAILURE);
	}

	printf("poll() returned: %d\n", ready);

	for(j = 0; j < num_pipes; j++) {
		if(poll_fd[j].revents & POLLIN) {
			printf("Readable: %d %3d\n", j, poll_fd[j].fd);
		}
	}

	exit(EXIT_FAILURE);
}
