#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "buffer.h"

static void ensure_allocated(buf_writer_t *writer, size_t n)
{
	while ((writer->cursor + n >= writer->buf_len - 1))
	{
		writer->buf_len *= 2;
		writer->buf = (wchar_t *)realloc(writer->buf, writer->buf_len * sizeof(wchar_t));
	}
}

static void bufindent(buf_writer_t *writer)
{
	for (int i = 0; i < writer->indent; i++)
		bufcpy(writer, L"\t");
}

void bufccpy(buf_writer_t *writer, const wchar_t chr)
{
	ensure_allocated(writer, 1);
	memcpy(writer->buf + writer->cursor, &chr, 1 * sizeof(wchar_t));
	writer->cursor++;
}

void bufcpy(buf_writer_t *writer, const wchar_t *string)
{
	// printf("invoked bufcpy with \"%ls\"\n", string);
	size_t len = wcslen(string);
	ensure_allocated(writer, len);

	memcpy(writer->buf + writer->cursor, string, len * sizeof(wchar_t));
	writer->cursor += len;
}

void bufncpy(buf_writer_t *writer, const wchar_t *string)
{
	bufcpy(writer, string);
	ensure_allocated(writer, 8);
	writer->buf[writer->cursor++] = L'\n';
}

void bufend(buf_writer_t *writer)
{
	writer->buf = (wchar_t *)realloc(writer->buf, (writer->cursor + 1) * sizeof(wchar_t));
	writer->buf[writer->cursor] = L'\x00';
}
