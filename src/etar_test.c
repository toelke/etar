#include <stdio.h>
#include "../lib/etar.h"

#define WORKSPACESIZE 2048
static char workspace[WORKSPACESIZE];

int tar_new_data_callback(void *cls, const char *data, unsigned int len) {
	return 1;
}

int main(int c, char **v) {
	tar_t t;
	int res = tar_init(&t, NULL, workspace, WORKSPACESIZE);
	printf("tar_init returned %d\n", res);

	if (res != 0) return 1;

	return 0;
}
