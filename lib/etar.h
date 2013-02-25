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

#ifndef ETAR_H
#define ETAR_H

typedef struct tar_ctx {
	enum {
		HEADER,
		DATA,
		PADDING,
		ABORT
	} parsing_state;
	/** The amount of data covered in this state */
	unsigned int state_size;
	/** The amount of data still to be seed in this state */
	unsigned int state_left;
	char *workspace;
	unsigned int workspacesize;
	/** position of the write-pointer, number of valid bytes in workspace */
	unsigned int pos;
	void *cls;
} tar_t;

/**
 * @brief User-supplied callback. Gets called each time a new file is encountered in the archive
 *
 * @param cls Context for the user
 * @param filename The full path of the file
 * @param filesize The size of the file
 *
 * @return 0 if unpacking should stop now
 */
extern int tar_new_file_callback(void *cls, const char* filename, unsigned int filesize);

/**
 * @brief User-supplied callback. Gets called for each block of data in the current file
 *
 * @param cls Context for the user
 * @param data The data
 * @param len The length of the data (may be less than workspacesize!)
 *
 * @return 0 if unpacking should stop now
 */
extern int tar_new_data_callback(void *cls, const char *data, unsigned int len);

/**
 * @brief Initializes tar_t-Context
 *
 * @param t The tar-Context
 * @param cls Context for the user
 * @param workspace A piece of memory. Must be >= 512 bytes
 * @param workspacesize The size of the workspace
 *
 * @return 0 on success, -1 on error
 */
int tar_init(tar_t *t, void *cls, char *workspace, unsigned int workspacesize);

/**
 * @brief Provide the tar-code with the next (arbitrarily sized) block of data
 *
 * @param t The tar-Context
 * @param data The new data
 * @param len The size of the data
 *
 * @return 0 on success, -1 if an error occurred
 */
int tar_new_data(tar_t *t, const char *data, unsigned int len);

#endif
