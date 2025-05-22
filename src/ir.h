#pragma once

#include "buffer.h"
#include "lexer.h"
#include "list.h"
#include "parser.h"
#include <stdlib.h>

#define FMT_BUF_MAX_LEN 512
#define DEFAULT_IDENTIFIER_ALLOC 256
#define DEFAULT_ASM_ALLOC 256
#define LINE_BUF_SZ 128

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static character_t *ENTRYPOINT_NAME = L"_start";

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

typedef enum IR_INSTRUCTION
{
	IR_INVALID = 0,
	IR_CONST,
	IR_ASSIGN,
	IR_VARDECL,
	IR_BRANCH_UNCONDITIONAL,
	IR_BRANCH_CONDITIONAL,
	IR_CALL,
	IR_RET,
	IR_NATIVE_ASM,
	IR_BINARY_ADD,
	IR_BINARY_SUB,
	IR_BINARY_MUL,
	IR_BINARY_DIV,
	IR_BINARY_XOR,
	IR_BINARY_OR,
	IR_BINARY_AND,
	IR_BINARY_EQU,
	IR_BINARY_LESS,
	IR_BINARY_GREATER,
	IR_LABEL
} ir_instruction_type_t;

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
	character_t *function_name;

	union {
		function_t function;
		variable_t variable;
	} value;
} identifier_t;

typedef struct translator_ctx
{
	int scope_var_cnt;
	character_t *current_function;
} translator_ctx_t;

typedef struct ir_operand
{
	int var_idx;
	character_t *label;
	int64_t value;
} ir_operand_t;

typedef struct ir_instruction
{
	ir_instruction_type_t type;
	int operand_cnt;
	ir_operand_t *operands;
} ir_instruction_t;

typedef struct ir_function
{
	int arg_cnt;
	int var_cnt;
	wchar_t *name;
	list_t instructions;
} ir_function_t;


typedef struct ir_program
{
	list_t functions;
} ir_program_t;


wchar_t *translate_to_ir(ast_node_t *ast);
ir_program_t parse_ir(const wchar_t *ir_buf);
