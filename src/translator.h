#pragma once

#include <stdlib.h>
#include "parser.h"
#include "buffer.h"

static const size_t DEFAULT_ASM_ALLOC = 256;

buf_writer_t translate(ASTNode* ast);
