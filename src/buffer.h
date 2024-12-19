#pragma once

#include <stdlib.h>

typedef struct buf_writer
{
	char* buf;
	size_t buf_len;
	size_t cursor;
	size_t indent;
} buf_writer_t;

void bufcpy(buf_writer_t* writer, const char* string);
void bufncpy(buf_writer_t* writer, const char* string);
