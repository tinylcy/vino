#include <stdio.h>
#include <sys/stat.h>

int main(void) {
	struct stat info;
	char pathname[] = "../cgi-bin/myscript.sh";
	printf("exist: %d\n", stat(pathname, &info) == 0);
}
