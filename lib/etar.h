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
