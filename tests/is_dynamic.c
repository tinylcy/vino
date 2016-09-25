#include <stdio.h>
#include <string.h>

int main(void) {
	char uri[] = "./cgi-bin/myscript.cgi?a=8&b=24";
	if(strstr(uri, "cgi-bin") != NULL) {
		printf("contains!\n");
	}
}
