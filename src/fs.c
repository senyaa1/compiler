#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <wchar.h>

#include "fs.h"

size_t read_file(const char* filepath, char** content)
{
	int fd = open(filepath, O_RDONLY);
	if (fd == -1)
		return 0;

	struct stat st = {0};

	fstat(fd, &st);
	off_t sz = st.st_size;

	char* fileptr = (char*)mmap(NULL, sz, PROT_READ, MAP_PRIVATE, fd, 0);
	*content = (char*)calloc(sz + 1, sizeof(char));
	memcpy(*content, fileptr, sz);

	munmap(fileptr, sz);
	close(fd);

	return sz;
}

int write_file(const char* filepath, char* content, size_t size)
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
