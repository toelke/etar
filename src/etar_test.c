/* Copyright (c) 2013, Philipp Tölke
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../lib/etar.h"

#define WORKSPACESIZE 2048
static char workspace[WORKSPACESIZE];

int tar_new_file_callback(void *cls, const char* filename, unsigned int filesize) {
	printf("New file: %s; size: %d\n", filename, filesize);
	return 1;
}

int tar_new_data_callback(void *cls, const char *data, unsigned int len) {
	printf("New data; size: %d\n", len);
	return 1;
}

int main(int c, char **v) {
	tar_t t;
	int res = tar_init(&t, NULL, workspace, WORKSPACESIZE);
	printf("tar_init returned %d\n", res);
	if (res != 0) return 1;

	int f = open("t.tar", O_RDONLY);
	if (f < 0) {
		printf("Could not open %s.\n", "t.tar");
		return 1;
	}

	char buf[10240];

	for(;;) {
		int r = read(f, buf, 10240);
		if (r <= 0) {
			printf("Read returned %d\n", r);
			break;
		}
		printf("read %d bytes.\n", r);
		r = tar_new_data(&t, buf, r);
		if (r < 0) break;
	}

	return 0;
}
