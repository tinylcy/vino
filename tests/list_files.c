#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>

/*
 * list all files in directory 'dirpath'
 */
static void listFiles(const char *dirpath) {
	
	DIR *dirp;
	struct dirent *dp;
	int is_current;    /* true if 'dirpath' is '.' */

	is_current = strcmp(dirpath, ".") == 0 ? 1 : 0;

	dirp = opendir(dirpath);
	if(dirp == NULL) {
		perror("opendir");
		exit(1);
	}

	/* for each entry in this directory, print directory + filename */
	for(;;) {
		errno = 0;    /* to distinguish error from end-of-directory */
		dp = readdir(dirp);
		if(dp == NULL) {
			break;
		}

		if(strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
			continue;    /* skip . and .. */
		}

		if(!is_current) {
			printf("%s/", dirpath);
		}
		printf("%s\n", dp->d_name);
	}

	if(errno != 0) {
		perror("readdir");
		exit(1);
	}

	if(closedir(dirp) == -1) {
		perror("closedir");
		exit(1);
	}
}

int main(int argc, char const *argv[]) {
	
	if(argc > 1 && strcmp(argv[1], "--help") == 0) {
		printf("usage: %s [dir...]\n", argv[0]);
		return 0;
	}

	if(argc == 1) {
		listFiles(".");
	} else {
		for(argv++; *argv; argv++) {
			listFiles(*argv);
		}
	}

	return 0;
}