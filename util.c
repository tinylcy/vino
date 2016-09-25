#include <sys/stat.h>

/*-------------------------------------------------------*
	check whether the request resource is a directory.
  -------------------------------------------------------*/
int is_directory(const char *filepath) {
	struct stat info;
	if(stat(filepath, &info) == 0 && S_ISDIR(info.st_mode)) {
		return 1;
	}
	return 0;
}

/*-------------------------------------------------------*
	check if a file is executable.
  -------------------------------------------------------*/
int is_executable(const char *filepath) {
	struct stat info;
	return (stat(filepath, &info) == 0 && S_ISREG(info.st_mode) && (S_IXUSR & info.st_mode));
}

/*-------------------------------------------------------*
	check if a file is readable.
  -------------------------------------------------------*/
int is_readable(const char *filepath) {
	struct stat info;
	return ((stat(filepath, &info) == 0) && S_ISREG(info.st_mode) && (S_IRUSR & info.st_mode));
}

/*-------------------------------------------------------*
	check if a file exists.
  -------------------------------------------------------*/
int file_exist(const char *filepath) {
	struct stat info;
	return (stat(filepath, &info) == 0);
}


