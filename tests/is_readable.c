#include <stdio.h>
#include <sys/stat.h>

int main(void) {
	struct stat info;
	char *filepath = "../html/index.html";
	if((stat(filepath, &info) == 0) && S_ISREG(info.st_mode) && (S_IRUSR & info.st_mode)) {
		printf("readable\n");
	} else {
		printf("unreadable\n");
	}

	return 0;
}
