#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wchar.h>

#include "fs.h"

size_t read_file(const char *filepath, wchar_t **content)
{
	printf("opening %s\n", filepath);
	if (!filepath || !content)
		return 0;

	int fd = open(filepath, O_RDONLY);
	if (fd == -1)
		return 0;

	struct stat st;
	if (fstat(fd, &st) == -1)
	{
		close(fd);
		return 0;
	}

	off_t sz = st.st_size;

	char *fileptr = mmap(NULL, sz, PROT_READ, MAP_PRIVATE, fd, 0);
	if (fileptr == MAP_FAILED)
	{
		close(fd);
		return 0;
	}

	// char *mbuf = calloc(sz + 1, sizeof(char));
	// memcpy(mbuf, fileptr, sz);
	// mbuf[sz] = '\0';

	size_t wlen = mbstowcs(NULL, fileptr, 0);
	if (wlen == -1)
	{ 
		// free(fileptr);
		munmap(fileptr, sz);
		close(fd);
		return 0;
	}

	*content = calloc(wlen + 1, sizeof(wchar_t));

	mbstowcs(*content, fileptr, wlen + 1);

	// free(mbuf);
	munmap(fileptr, sz);
	close(fd);

	return wlen; 
}


int write_file(const char *filepath, char *content, size_t size)
{
	FILE *file = fopen(filepath, "wb");

	if (file == NULL)
	{
		fprintf(stderr, "Error opening file %s\n", filepath);
		return -1;
	}

	size_t bytes_written = fwrite(content, 1, size, file);

	if (bytes_written != size)
	{
		fprintf(stderr, "Error writing to file %s\n", filepath);
		fclose(file);
		return -1;
	}

	fclose(file);

	return 0;
}
