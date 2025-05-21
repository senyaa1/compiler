#pragma once

#include <stdint.h>
#include <stdlib.h>

size_t read_file(const char* filepath, wchar_t** content);
int write_file(const char *filepath, wchar_t *content, size_t size);
