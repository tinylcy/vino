#include <stdio.h>
#include <sys/stat.h>

int main(void) {
	char pathname[] = "./html/";
	struct stat info;
	if(stat(pathname, &info) == 0 && S_ISDIR(info.st_mode)) {
		printf("%s is a directory\n", pathname);
	}

	return 0;
}
