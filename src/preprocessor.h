#pragma once

#include <stdlib.h>
#include "lexer.h"

#define MAX_LINE_LENGTH 1024

static const size_t MAX_INCLUDE_DEPTH = 50;
static const size_t DEFAULT_PREPROCESSOR_ALLOC = 256;
static const character_t* INCLUDE_STATEMENT = "#include";

int preprocess(const char* file_path, char** buffer);
