#pragma once

#include <stdint.h>
#include <stdlib.h>

size_t read_file(const char* filepath, char** content);
int write_file(const char* filepath, char* content, size_t size);
