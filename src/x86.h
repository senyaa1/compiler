#pragma once

#include "lexer.h"
#include "list.h"
#include <wchar.h>

#define INT_SZ 8

typedef enum VAR_ASSIGNMENTS
{
	STACK = 0,
	// REG_RAX,
	// REG_RBX,
	REG_RCX = 1,
	REG_RDX = 2,
	REG_RSI = 3,
	REG_RDI = 4,
	REG_R8 = 5,
	REG_R9 = 6,
	REG_R10 = 7,
	REG_R11 = 8,
	REG_R12 = 9,
	REG_R13 = 10,
	REG_R14 = 11,
	REG_R15 = 12,
} assignment_type_t;

typedef struct var_assignment
{
	assignment_type_t type;
	int stack_idx;
	character_t *current_reg;
} var_assignment_t;

static character_t *x86_regs[] = {
    L"rax", L"rbx", L"rcx", L"rdx", L"rsi", L"rdi", L"r8", L"r9", L"r10", L"r11", L"r12", L"r13", L"r14", L"r15",
};

static character_t *reg_names[] = {
    L"rcx", L"rdx", L"rsi", L"rdi", L"r8", L"r9", L"r10", L"r11", L"r12", L"r13", L"r14", L"r15",
};

static character_t *m8_reg_names[] = {
    L"al", L"bl", L"cl", L"dl", L"sil", L"dil", L"r8b", L"r9b", L"r10b", L"r11b", L"r12b", L"r13b", L"r14b", L"r15b",
};

wchar_t *translate_ir_to_x86(const wchar_t *ir_buf);
