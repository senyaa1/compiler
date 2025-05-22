#pragma once

#include "lexer.h"
#include <stdlib.h>

#define PATH_SEP L'/'

#define MAX_LINE 4096
#define MAX_DEPTH 32

int preprocess(const char *file_path, wchar_t **buffer);
	
