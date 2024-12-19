#include <stdlib.h>
#include <string.h>

#include "buffer.h"

static void ensure_allocated(buf_writer_t* writer, size_t n)
{
	while((writer->cursor + n >= writer->buf_len - 1))
	{
		writer->buf_len *= 2;
		writer->buf = (char*)realloc(writer->buf, writer->buf_len * sizeof(char));
	}
}

static void bufindent(buf_writer_t* writer)
{
	for(int i = 0; i < writer->indent; i++)	
		bufcpy(writer, "\t");
}

void bufcpy(buf_writer_t* writer, const char* string)
{
	size_t len = strlen(string);
	ensure_allocated(writer, len);

	memcpy(writer->buf + writer->cursor, string, len);
	writer->cursor += len;
}

void bufncpy(buf_writer_t* writer, const char* string)
{
	bufindent(writer);
	bufcpy(writer, string);
	writer->buf[writer->cursor++] = '\n';
}
