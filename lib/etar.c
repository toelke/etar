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

__attribute__ ((packed))
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
};

static void consume(tar_t *t, int len) {
	memmove(t->workspace + len, t->workspace, t->workspacesize - len);
	t->pos -= len;
	t->state_left -= len;
}

static unsigned int max_valid_data(tar_t *t) {
	return MIN(MIN(t->workspacesize, t->state_left), t->pos);
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
		if (0 == t->state_left) {
			t->parsing_state = PADDING;
			t->state_left = t->state_size = (512 - (t->state_size % 512) % 512);
		}
		break;
	}
	case PADDING: {
		consume(t, new_used);
		if (0 == t->state_left) {
			t->parsing_state = HEADER;
			t->state_left = t->state_size = 512;
		}
		break;
	}
	case HEADER: {
		if (new_used != 512) break;
		if (0 == t->state_left) {
			t->parsing_state = DATA;
			t->state_left = t->state_size = 512;
		}
		/**
		 * The checksum is calculated by taking the sum of the unsigned byte
		 * values of the header record with the eight checksum bytes taken to
		 * be ascii spaces (decimal value 32). It is stored as a six digit
		 * octal number with leading zeroes followed by a NUL and then a space.
		 */
		/* Numeric values are encoded in octal numbers using ASCII digits, with
 		 * leading zeroes. For historical reasons, a final NUL or space
		 * character should be used.
		 **/
	}
	}
}

int tar_init(tar_t *t, void *cls, char *workspace, unsigned int workspacesize) {
	t->workspace = workspace;
	t->workspacesize = workspacesize;
	t->pos = 0;
	t->parsing_state = HEADER;
	t->cls = cls;
	return 0;
}

int tar_new_data(tar_t *t, const char *data, unsigned int len) {
	int used = 0;
	{
		int new_used = MAX(len, t->workspacesize - t->pos);
		memcpy(t->workspace + t->pos, data, new_used);
		len -= new_used;
		data += new_used;
		used += new_used;
		do_state(t);
		if (t->parsing_state == ABORT) return -1;
	} while (len > 0);
	return 0;
}
