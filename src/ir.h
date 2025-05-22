#pragma once

#include "buffer.h"
#include "lexer.h"
#include "parser.h"
#include <stdlib.h>

#define FMT_BUF_MAX_LEN 512
#define DEFAULT_IDENTIFIER_ALLOC 256
#define DEFAULT_ASM_ALLOC 256

typedef enum SCOPE
{
	GLOBAL,
	LOCAL,
	ARG
} scope_t;

typedef enum IDENTIFIER_TYPE
{
	TYPE_VARIABLE,
	TYPE_FUNCTION,
} identifier_type_t;

typedef struct function
{
	character_t *label;
	size_t arg_cnt;
} function_t;

typedef struct variable
{
	scope_t scope;
	character_t *label;
	int var_idx;
} variable_t;

typedef struct identifier
{
	identifier_type_t type;
	character_t *identifier;

	union {
		function_t function;
		variable_t variable;
	} value;
} identifier_t;

typedef struct translator_ctx
{
	int scope_var_cnt;
	character_t* current_function;
} translator_ctx_t;

wchar_t* translate_to_ir(ast_node_t *ast);
