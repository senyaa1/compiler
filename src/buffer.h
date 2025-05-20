#pragma once

#include <stdlib.h>
#include <wchar.h>

typedef struct buf_writer
{
	wchar_t *buf;
	size_t buf_len;
	size_t cursor;
	size_t indent;
} buf_writer_t;

void bufcpy(buf_writer_t *writer, const wchar_t *string);
void bufncpy(buf_writer_t *writer, const wchar_t *string);
void bufend(buf_writer_t *writer);
void bufccpy(buf_writer_t *writer, const wchar_t chr);
