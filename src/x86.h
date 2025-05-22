#pragma once

#include "lexer.h"
#include "list.h"
#include <wchar.h>

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
} ir_instruction_type_t;

typedef struct ir_operand {
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

wchar_t *translate_ir_to_x86(const wchar_t *ir_buf);
