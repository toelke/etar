/*
 * Copyright (c) 2013, Philipp Tölke
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

#ifdef DEBUG
#include <stdio.h>
#endif

#include <string.h>
#include "etar.h"

#define MAX(a,b) (a) > (b) ? (a) : (b)
#define MIN(a,b) (a) < (b) ? (a) : (b)

struct TarFileHeader {
    char fileName[100];            //!< File name
    char fileMode[8];              //!< File mode
    char uid[8];                   //!< Owner User ID
    char gid[8];                   //!< Owner Group ID
    char fileSize[12];             //!< File size in bytes (octal format)
    char fileModificationTime[12]; //!< Modification time (unix timestamp of the file. (octal format)
    char headerChecksum[8];        //!< Checksum of the header.
    char linkIndicatorOrUstarType; //!< Link indicator (legacy format) or type fild (ustar format)
    char linkedFileName[100];      //!< File name of the linked file.
    char ustarIndicator[6];        //!< magic field indicating whether the ustar extension is present.
    char ustarVersion[2];          //!< The ustar version
    char ownerUserName[32];        //!< User name of the file owner
    char ownerGroupName[32];       //!< Group name of the file owner
    char deviceMajorNumer[8];      //!< Device major number
    char deviceMinorNumber[8];     //!< Device minor number
    char fileNamePrefix[155];      //!< File name prefix
    char _padding[12];             //!< Padding bytes
} __attribute__ ((packed));

static unsigned int oct2uint(const char* str, unsigned int len) {
	unsigned int ret = 0;
	unsigned int i;
	for (i = 0; i < len; i++) {
		char c = str[i];
		if (!c) break;
		ret *= 8;
		ret += c - '0';
	}
	return ret;
}

static void consume(tar_t *t, int len) {
	if (!len) return;
	memmove(t->workspace, t->workspace + len, t->workspacesize - len);
	t->pos -= len;
	t->state_left -= len;
}

static unsigned int max_valid_data(tar_t *t) {
	return MIN(MIN(t->workspacesize, t->state_left), t->pos);
}

static char check_hdr_checksum(struct TarFileHeader *hdr) {
	/**
	 * From wikipedia:
	 *
	 * The checksum is calculated by taking the sum of the unsigned byte
	 * values of the header record with the eight checksum bytes taken to
	 * be ascii spaces (decimal value 32). It is stored as a six digit
	 * octal number with leading zeroes followed by a NUL and then a space.
	 */
	unsigned int chksum = oct2uint(hdr->headerChecksum, sizeof hdr->headerChecksum);
	/* Set the checksum-field to spaces */
	memset(hdr->headerChecksum, 32, sizeof hdr->headerChecksum);
	unsigned int c_chksum = 0;
	unsigned int i;
	unsigned char* buf = (unsigned char*)hdr;
	for (i = 0; i < 512; i++)
		c_chksum += buf[i];
	return c_chksum == chksum;
}

static void do_state(tar_t *t) {
	int new_used = max_valid_data(t);
	switch(t->parsing_state) {
	case DATA: {
		int res = tar_new_data_callback(t->cls, t->workspace, new_used);
		if (!res) {
			t->parsing_state = ABORT;
			break;
		}
		consume(t, new_used);
		if (0 == t->state_left) {
			t->parsing_state = PADDING;
			t->state_left = t->state_size = (512 - (t->state_size % 512) % 512);
		}
		break;
	}
	case PADDING: {
#ifdef DEBUG
		printf("do_state: PADDING; state_left = %d, new_used = %d\n", t->state_left, new_used);
#endif
		consume(t, new_used);
		if (0 == t->state_left) {
			t->parsing_state = HEADER;
			t->state_left = t->state_size = 512;
		}
		break;
	}
	case HEADER: {
#ifdef DEBUG
		printf("do_state: HEADER; state_left = %d, new_used = %d\n", t->state_left, new_used);
#endif
		struct TarFileHeader *hdr = (struct TarFileHeader*) t->workspace;
		if (new_used < 512) break;
		if (hdr->fileName[0] == 0) {
#ifdef DEBUG
			printf("do_state: HEADER: empty filename\n");
#endif
			t->numempty++;
			consume(t, 512);
			t->parsing_state = HEADER;
			t->state_left = t->state_size = 512;
			break;
		}
		t->numempty = 0;
		if (!check_hdr_checksum(hdr)) {
#ifdef DEBUG
			printf("do_state: HEADER; invalid checksum fn = %s\n", hdr->fileName);
#endif
			t->parsing_state = ABORT;
			break;
		}
		unsigned int size = oct2uint(hdr->fileSize, sizeof hdr->fileSize);
#ifdef DEBUG
		printf("do_state: HEADER; filesize = %d\n", size);
#endif
		if ((hdr->linkIndicatorOrUstarType != 0 && hdr->linkIndicatorOrUstarType != '0') || hdr->fileName[strlen(hdr->fileName) - 1] == '/') {
			consume(t, 512);
			t->parsing_state = PADDING;
			unsigned int padding = 0;
			if (size % 512) {
				padding = 512 - (size % 512);
			}
			t->state_left = t->state_size = size + padding;
			break;
		}
		int res = tar_new_file_callback(t->cls, hdr->fileName, size);
		if (!res) {
			t->parsing_state = ABORT;
			break;
		}
		consume(t, 512);
		t->parsing_state = DATA;
		t->state_left = t->state_size = size;
		break;
	}
	default: {
		break;
	}
	}
}

int tar_init(tar_t *t, void *cls, char *workspace, unsigned int workspacesize) {
	t->workspace = workspace;
	t->workspacesize = workspacesize;
	t->pos = 0;
	t->parsing_state = HEADER;
	t->state_size = t->state_left = 512;
	t->cls = cls;
	t->numempty = 0;
	return 0;
}

int tar_new_data(tar_t *t, const char *data, unsigned int len) {
	int used = 0;
	do {
#ifdef DEBUG
		printf("tar_new_data: len = %d\n", len);
#endif
		int new_used = MIN(len, t->workspacesize - t->pos);
		if (new_used) {
			memcpy(t->workspace + t->pos, data, new_used);
			len -= new_used;
			data += new_used;
			used += new_used;
			t->pos += new_used;
		}
#ifdef DEBUG
		printf("tar_new_data: before call: state_left = %d, state = %d\n", t->state_left, t->parsing_state);
#endif
		do_state(t);
#ifdef DEBUG
		printf("tar_new_data: after call: state_left = %d, state = %d\n", t->state_left, t->parsing_state);
#endif
		if (t->parsing_state == ABORT) return -1;
		if (t->numempty >= 2) return -2;
	} while (len > 0);

	while (t->state_left < t->pos)
		do_state(t);

	return 0;
}
