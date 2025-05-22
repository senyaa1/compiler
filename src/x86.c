#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#include "buffer.h"
#include "ir.h"
#include "lexer.h"
#include "list.h"
#include "x86.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))


static void skip_ws(const wchar_t **p, const wchar_t *end)
{
	while (*p < end && iswspace(**p))
		(*p)++;
}

static int expect_kw(const wchar_t **p, const wchar_t *end, const wchar_t *kw)
{
	skip_ws(p, end);
	size_t len = wcslen(kw);
	if (*p + len <= end && wcsncmp(*p, kw, len) == 0)
	{
		*p += len;
		return 1;
	}
	return 0;
}

static wchar_t *parse_ident(const wchar_t **p, const wchar_t *end)
{
	skip_ws(p, end);
	const wchar_t *start = *p;
	while (*p < end && (iswalnum(**p) || **p == L'_' || **p == L'-'))
		(*p)++;
	size_t len = *p - start;
	wchar_t *str = calloc(len + 1, sizeof(wchar_t));
	wcsncpy(str, start, len);
	return str;
}

static wchar_t *parse_string(const wchar_t **p, const wchar_t *end)
{
	skip_ws(p, end);
	const wchar_t *start = *p;
	while (*p < end && **p != L'\"')
		(*p)++;
	size_t len = *p - start;
	wchar_t *str = calloc(len + 1, sizeof(wchar_t));
	wcsncpy(str, start, len);
	return str;
}

static int parse_int(const wchar_t **p, const wchar_t *end)
{
	skip_ws(p, end);
	int sign = 1;
	if (*p < end && **p == L'-')
	{
		sign = -1;
		(*p)++;
	}
	int val = 0;
	while (*p < end && iswdigit(**p))
	{
		val = val * 10 + (**p - L'0');
		(*p)++;
	}
	return sign * val;
}

ir_program_t parse_ir(const wchar_t *ir_buf)
{
	size_t len = wcslen(ir_buf);

	ir_program_t res;
	list_ctor(&res.functions, 8);
	const wchar_t *p = ir_buf;
	const wchar_t *end = ir_buf + len;

	while (p < end && *p)
	{
		skip_ws(&p, end);
		if (expect_kw(&p, end, L"function"))
		{
			skip_ws(&p, end);
			ir_function_t *fn = calloc(1, sizeof(ir_function_t));
			fn->name = parse_ident(&p, end);
			expect_kw(&p, end, L"(");
			fn->arg_cnt = 0;
			while (!expect_kw(&p, end, L")"))
			{
				wchar_t *type = parse_ident(&p, end);
				free(type);

				skip_ws(&p, end);

				if (p < end && *p == L'%')
				{
					p++;
					parse_int(&p, end);
				}

				fn->arg_cnt++;
				skip_ws(&p, end);

				if (p < end && *p == L',')
					p++;
			}

			expect_kw(&p, end, L"{");
			list_ctor(&fn->instructions, 16);

			while (p < end && *p)
			{
				skip_ws(&p, end);
				if (p < end && *p == L'}')
				{
					p++;
					skip_ws(&p, end);
					fn->var_cnt = parse_int(&p, end);
					break;
				}
				if (iswalnum(*p) || *p == L'_')
				{
					const wchar_t *q = p;
					while (q < end && *q != L':' && *q != L'\n')
						q++;
					if (q < end && *q == L':')
					{
						ir_instruction_t *inst = calloc(1, sizeof(ir_instruction_t));
						inst->type = IR_LABEL;
						inst->operands = calloc(1, sizeof(ir_operand_t));
						inst->operands[0].label = parse_ident(&p, end);
						list_insert_tail(&fn->instructions, inst);
						p = q + 1;
						continue;
					}
				}
				ir_instruction_t *inst = calloc(1, sizeof(ir_instruction_t));
				int cur_assignment = 0;
				if (expect_kw(&p, end, L"%"))
				{
					cur_assignment = parse_int(&p, end);
					skip_ws(&p, end);
					expect_kw(&p, end, L"=");
					skip_ws(&p, end);
				}
				if (expect_kw(&p, end, L"add"))
				{
					inst->type = IR_BINARY_ADD;
				}
				else if (expect_kw(&p, end, L"sub"))
				{
					inst->type = IR_BINARY_SUB;
				}
				else if (expect_kw(&p, end, L"mul"))
				{
					inst->type = IR_BINARY_MUL;
				}
				else if (expect_kw(&p, end, L"div"))
				{
					inst->type = IR_BINARY_DIV;
				}
				else if (expect_kw(&p, end, L"and"))
				{
					inst->type = IR_BINARY_AND;
				}
				else if (expect_kw(&p, end, L"or"))
				{
					inst->type = IR_BINARY_OR;
				}
				else if (expect_kw(&p, end, L"xor"))
				{
					inst->type = IR_BINARY_XOR;
				}
				else if (expect_kw(&p, end, L"equ"))
				{
					inst->type = IR_BINARY_EQU;
				}
				else if (expect_kw(&p, end, L"gt"))
				{
					inst->type = IR_BINARY_GREATER;
				}
				else if (expect_kw(&p, end, L"lt"))
				{
					inst->type = IR_BINARY_LESS;
				}
				else if (expect_kw(&p, end, L"const"))
				{
					inst->type = IR_CONST;
				}
				else if (expect_kw(&p, end, L"assign"))
				{
					inst->type = IR_ASSIGN;
				}
				else if (expect_kw(&p, end, L"call"))
				{
					inst->type = IR_CALL;
				}
				else if (expect_kw(&p, end, L"ret"))
				{
					inst->type = IR_RET;
				}
				else if (expect_kw(&p, end, L"br"))
				{
					inst->type = IR_BRANCH_UNCONDITIONAL;
				}
				else if (expect_kw(&p, end, L"bc"))
				{
					inst->type = IR_BRANCH_CONDITIONAL;
				}
				else if (expect_kw(&p, end, L"native_asm"))
				{
					inst->type = IR_NATIVE_ASM;
				}
				else
				{
					inst->type = IR_INVALID;
				}

				skip_ws(&p, end);

				if (inst->type == IR_CONST)
				{
					inst->operand_cnt = 2;
					inst->operands = calloc(inst->operand_cnt, sizeof(ir_operand_t));
					inst->operands[0].var_idx = cur_assignment;
					inst->operands[1].value = parse_int(&p, end);
				}
				else if (inst->type == IR_BRANCH_UNCONDITIONAL)
				{
					inst->operand_cnt = 1;
					inst->operands = calloc(inst->operand_cnt, sizeof(ir_operand_t));
					inst->operands[0].label = parse_ident(&p, end);
				}
				else if (inst->type == IR_BRANCH_CONDITIONAL)
				{
					inst->operand_cnt = 2;
					inst->operands = calloc(inst->operand_cnt, sizeof(ir_operand_t));

					expect_kw(&p, end, L"%");
					inst->operands[0].var_idx = parse_int(&p, end);
					expect_kw(&p, end, L",");

					inst->operands[1].label = parse_ident(&p, end);
				}
				else if (inst->type == IR_NATIVE_ASM)
				{
					inst->operand_cnt = 1;
					inst->operands = calloc(inst->operand_cnt, sizeof(ir_operand_t));
					expect_kw(&p, end, L"\"");
					inst->operands[0].label = parse_string(&p, end);
				}
				else if (inst->type == IR_CALL)
				{
					character_t *call_label = parse_ident(&p, end);
					int arg_count = parse_int(&p, end);

					inst->operand_cnt = arg_count + 2;
					inst->operands = calloc(inst->operand_cnt, sizeof(ir_operand_t));
					inst->operands[0].var_idx = cur_assignment;
					inst->operands[1].label = call_label;
					for (int i = 0; i < arg_count; i++)
					{
						expect_kw(&p, end, L"%");
						inst->operands[2 + i].var_idx = parse_int(&p, end);
					}
				}
				else if (inst->type == IR_RET)
				{
					expect_kw(&p, end, L"%");
					inst->operand_cnt = 1;
					inst->operands = calloc(inst->operand_cnt, sizeof(ir_operand_t));
					inst->operands[0].var_idx = parse_int(&p, end);
				}
				else if (inst->type == IR_BINARY_ADD || inst->type == IR_BINARY_SUB ||
					 inst->type == IR_BINARY_MUL || inst->type == IR_BINARY_DIV ||
					 inst->type == IR_BINARY_AND || inst->type == IR_BINARY_OR ||
					 inst->type == IR_BINARY_XOR || inst->type == IR_BINARY_GREATER ||
					 inst->type == IR_BINARY_LESS || inst->type == IR_BINARY_EQU)
				{
					inst->operand_cnt = 3;
					inst->operands = calloc(inst->operand_cnt, sizeof(ir_operand_t));

					inst->operands[0].var_idx = cur_assignment;

					expect_kw(&p, end, L"%");
					inst->operands[1].var_idx = parse_int(&p, end);
					skip_ws(&p, end);
					expect_kw(&p, end, L",");

					skip_ws(&p, end);
					expect_kw(&p, end, L"%");
					inst->operands[2].var_idx = parse_int(&p, end);
				}
				else if (inst->type == IR_ASSIGN || inst->type == IR_VARDECL)
				{
					expect_kw(&p, end, L"%");
					inst->operand_cnt = 2;
					inst->operands = calloc(inst->operand_cnt, sizeof(ir_operand_t));
					inst->operands[0].var_idx = cur_assignment;
					inst->operands[1].var_idx = parse_int(&p, end);
				}

				while (p < end && *p != L'\n')
					p++;

				if (p < end && *p == L'\n')
					p++;

				list_insert_tail(&fn->instructions, inst);
			}
			list_insert_tail(&res.functions, fn);
		}
		else
		{
			p++;
		}
	}
	return res;
}

void ir_program_free(ir_program_t prog)
{
	list_elem_t *fe = list_begin(&prog.functions);
	while (fe != list_end(&prog.functions))
	{
		ir_function_t *fn = (ir_function_t *)fe->data;
		free(fn->name);
		list_elem_t *ie = list_begin(&fn->instructions);
		while (ie != list_end(&fn->instructions))
		{
			ir_instruction_t *instr = ie->data;
			for (int i = 0; i < instr->operand_cnt; i++)
				free(instr->operands[i].label);

			free(instr->operands);

			free((ir_instruction_t *)ie->data);
			ie = list_next(&fn->instructions, ie);
		}
		list_dtor(&fn->instructions);
		free(fn);
		fe = list_next(&prog.functions, fe);
	}
	list_dtor(&prog.functions);
}

/* calling convention-
 * args - stack
 * preserves all registers
 *	epilogue - mov rbp, rsp.
 *		now can access arguments with rbp+offset
 *		locals are at rbp-offset
 * return value - rax
 */

/* local assignments
 *
 *
 *
 *
 */

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


static character_t *x86_regs[] = {
    L"rax", L"rbx", L"rcx", L"rdx", L"rsi", L"rdi", L"r8", L"r9", L"r10", L"r11", L"r12", L"r13", L"r14", L"r15",
};

static character_t *reg_names[] = {
    L"rcx", L"rdx", L"rsi", L"rdi", L"r8", L"r9", L"r10", L"r11", L"r12", L"r13", L"r14", L"r15",
};

static character_t *m8_reg_names[] = {
    L"al", L"bl", L"cl", L"dl", L"sil", L"dil", L"r8b", L"r9b", L"r10b", L"r11b", L"r12b", L"r13b", L"r14b", L"r15b",
};

static character_t *get_corresponding_m8_register(character_t *reg)
{
	for (int i = 0; i < sizeof(x86_regs) / sizeof(character_t *); i++)
	{
		if (wcscmp(reg, x86_regs[i]))
			continue;

		return m8_reg_names[i];
	}
	return 0;
}


typedef struct var_assignment
{
	assignment_type_t type;
	int stack_idx;
	character_t *current_reg;
} var_assignment_t;

#define INT_SZ 8

static void assign_registers(buf_writer_t *writer, var_assignment_t *assignment, bool to_rcx)
{
	if (assignment->type != STACK)
	{
		assignment->current_reg = reg_names[assignment->type - 1];
		return;
	}

	character_t fmt_buf[FMT_BUF_MAX_LEN] = {0};
	swprintf(fmt_buf, FMT_BUF_MAX_LEN, (to_rcx ? L"mov rcx, [rbp%c%d]" : L"mov rbx, [rbp%c%d]"),
		 (assignment->stack_idx >= 0 ? '-' : '+'), abs(assignment->stack_idx * INT_SZ));
	assignment->current_reg = (to_rcx ? L"rcx" : L"rbx");
	bufncpy(writer, fmt_buf);
}

static void write_back_if_needed(buf_writer_t *writer, var_assignment_t *assignment, bool to_rcx)
{
	if (assignment->type != STACK)
		return;

	character_t fmt_buf[FMT_BUF_MAX_LEN] = {0};
	swprintf(fmt_buf, FMT_BUF_MAX_LEN, (to_rcx ? L"mov [rbp%c%d], rcx" : L"mov [rbp%c%d], rbx"),
		 (assignment->stack_idx >= 0 ? '-' : '+'), abs(assignment->stack_idx * INT_SZ));
	bufncpy(writer, fmt_buf);
}

void print_assignment(var_assignment_t *assignment)
{
	printf("type: %d assigned reg: %ls or stack #%d\n", assignment->type, assignment->current_reg,
	       assignment->stack_idx);
}

#define TRANSLATE_BINARY_OP(IR_NAME, INSTRUCTION)                                                                      \
	case IR_NAME:                                                                                                  \
		{                                                                                                      \
			bufncpy(writer, L"; binary-binary-op");	\
			var_assignment_t res = assignments[inst->operands[0].var_idx];                                 \
			var_assignment_t op1 = assignments[inst->operands[1].var_idx];                                 \
			var_assignment_t op2 = assignments[inst->operands[2].var_idx];                                 \
                                                                                                                       \
			assign_registers(writer, &res, false);                                                         \
			assign_registers(writer, &op1, true);                                                          \
			swprintf(fmt_buf, FMT_BUF_MAX_LEN, L"mov %ls, %ls", res.current_reg, op1.current_reg);         \
			bufncpy(writer, fmt_buf);                                                                      \
                                                                                                                       \
			assign_registers(writer, &op2, true);                                                          \
			swprintf(fmt_buf, FMT_BUF_MAX_LEN, INSTRUCTION L" %ls, %ls", res.current_reg,                  \
				 op2.current_reg);                                                                     \
			bufncpy(writer, fmt_buf);                                                                      \
			write_back_if_needed(writer, &res, false);                                                     \
		}                                                                                                      \
		break;


#define TRANSLATE_BOOLEAN_OP(IR_NAME, SET_CONDITION)                                                                   \
	case IR_NAME:                                                                                                  \
		{                                                                                                      \
			var_assignment_t res = assignments[inst->operands[0].var_idx];                                 \
			var_assignment_t op1 = assignments[inst->operands[1].var_idx];                                 \
			var_assignment_t op2 = assignments[inst->operands[2].var_idx];                                 \
                                                                                                                       \
			assign_registers(writer, &res, false);                                                         \
			swprintf(fmt_buf, FMT_BUF_MAX_LEN, L"mov %ls, 0", res.current_reg);                            \
			bufncpy(writer, fmt_buf);                                                                      \
			write_back_if_needed(writer, &res, false);                                                     \
                                                                                                                       \
			assign_registers(writer, &op1, false);                                                         \
			assign_registers(writer, &op2, true);                                                          \
			swprintf(fmt_buf, FMT_BUF_MAX_LEN, L"cmp %ls, %ls", op1.current_reg, op2.current_reg);         \
			bufncpy(writer, fmt_buf);                                                                      \
                                                                                                                       \
			swprintf(fmt_buf, FMT_BUF_MAX_LEN, SET_CONDITION L" %ls",                               \
				 get_corresponding_m8_register(res.current_reg));                     \
			bufncpy(writer, fmt_buf);                                                                      \
			write_back_if_needed(writer, &res, false);                                                     \
                                                                                                                       \
			write_back_if_needed(writer, &res, false);                                                     \
		}                                                                                                      \
		break;

void translate_function(buf_writer_t *writer, ir_function_t *func)
{
	character_t fmt_buf[FMT_BUF_MAX_LEN] = {0};
	swprintf(fmt_buf, FMT_BUF_MAX_LEN, L"%ls:", func->name);
	bufncpy(writer, fmt_buf);

	// prologue
	bufncpy(writer, L"push rbp");
	bufncpy(writer, L"mov rbp, rsp");

	// generate variable assignments
	var_assignment_t *assignments = calloc(func->var_cnt + 1, sizeof(var_assignment_t));
	// first vars are args and they should be on the stack
	int reg_vars = 0, stack_vars = 0;
	for (int i = 0; i < func->var_cnt; i++)
	{
		if (i < func->arg_cnt)
		{
			assignments[i + 1].type = STACK;
			assignments[i + 1].stack_idx = -(i + 2);
			continue;
		}

		if (reg_vars < 12)
		{
			assignments[i + 1].type = i + 1;
			reg_vars++;
			continue;
		}
		assignments[i + 1].type = STACK;
		assignments[i + 1].stack_idx = i - 12;
		stack_vars++;
	}

	size_t total_used_stack = MAX(0, stack_vars) * INT_SZ;

	// allocate function stack frame
	if (total_used_stack)
	{
		swprintf(fmt_buf, FMT_BUF_MAX_LEN, L"sub rsp, %zu", total_used_stack);
		bufncpy(writer, fmt_buf);
	}

	for (int i = 0; i < MIN(12, func->var_cnt); i++)
	{
		swprintf(fmt_buf, FMT_BUF_MAX_LEN, L"push %ls", reg_names[i]);
		bufncpy(writer, fmt_buf);
	}

	for (list_elem_t *instr_node = list_begin(&func->instructions); instr_node;
	     instr_node = list_next(&func->instructions, instr_node))
	{
		ir_instruction_t *inst = instr_node->data;
		switch (inst->type)
		{
			case IR_NATIVE_ASM:
				bufncpy(writer, inst->operands[0].label);
				break;
			case IR_CONST:
				// bufncpy(writer, L"; const");
				var_assignment_t c1 = assignments[inst->operands[0].var_idx];
				assign_registers(writer, &c1, false);

				swprintf(fmt_buf, FMT_BUF_MAX_LEN, L"mov %ls, %ld", c1.current_reg,
					 inst->operands[1].value);
				bufncpy(writer, fmt_buf);

				write_back_if_needed(writer, &c1, false);
				break;
			case IR_ASSIGN:
				// bufncpy(writer, L"; assign");
				printf("%d %d\n", inst->operands[0].var_idx, inst->operands[1].var_idx);

				var_assignment_t a1 = assignments[inst->operands[0].var_idx];
				var_assignment_t a2 = assignments[inst->operands[1].var_idx];

				assign_registers(writer, &a1, false);
				assign_registers(writer, &a2, a1.type == STACK);

				// print_assignment(&a1);
				// print_assignment(&a2);

				swprintf(fmt_buf, FMT_BUF_MAX_LEN, L"mov %ls, %ls", a1.current_reg, a2.current_reg);
				bufncpy(writer, fmt_buf);

				write_back_if_needed(writer, &a1, false);
				break;
			case IR_RET: // prologue
				// bufncpy(writer, L"; return");
				// printf("%d\n", inst->operands[0].var_idx);
				var_assignment_t r1 = assignments[inst->operands[0].var_idx];

				assign_registers(writer, &r1, false);
				swprintf(fmt_buf, FMT_BUF_MAX_LEN, L"mov rax, %ls", r1.current_reg);
				bufncpy(writer, fmt_buf);

				for (int i = MIN(12, func->var_cnt) - 1; i >= 0; i--)
				{
					swprintf(fmt_buf, FMT_BUF_MAX_LEN, L"pop %ls", reg_names[i]);
					bufncpy(writer, fmt_buf);
				}

				bufncpy(writer, L"pop rbp");
				bufncpy(writer, L"ret");
				break;

			case IR_CALL:
				bufncpy(writer, L"; call");
				var_assignment_t retval = assignments[inst->operands[0].var_idx];
				character_t *call_label = inst->operands[1].label;

				for (int i = 2; i < inst->operand_cnt; i++)
				{
					var_assignment_t call_op = assignments[inst->operands[i].var_idx];
					assign_registers(writer, &call_op, false);
					swprintf(fmt_buf, FMT_BUF_MAX_LEN, L"push %ls", call_op.current_reg);
					bufncpy(writer, fmt_buf);
				}

				swprintf(fmt_buf, FMT_BUF_MAX_LEN, L"call %ls", call_label);
				bufncpy(writer, fmt_buf);

				for (int i = inst->operand_cnt - 1; i >= 2; i--)
				{
					swprintf(fmt_buf, FMT_BUF_MAX_LEN, L"pop rbx");
					bufncpy(writer, fmt_buf);
				}

				// save return value
				assign_registers(writer, &retval, false);
				swprintf(fmt_buf, FMT_BUF_MAX_LEN, L"mov %ls, rax", retval.current_reg);
				bufncpy(writer, fmt_buf);
				write_back_if_needed(writer, &retval, false);
				break;


				TRANSLATE_BINARY_OP(IR_BINARY_ADD, L"add");
				TRANSLATE_BINARY_OP(IR_BINARY_SUB, L"sub");
				TRANSLATE_BINARY_OP(IR_BINARY_MUL, L"mul");
				TRANSLATE_BINARY_OP(IR_BINARY_DIV, L"div");
				TRANSLATE_BINARY_OP(IR_BINARY_XOR, L"xor");
				TRANSLATE_BINARY_OP(IR_BINARY_OR, L"or");
				TRANSLATE_BINARY_OP(IR_BINARY_AND, L"and");

				TRANSLATE_BOOLEAN_OP(IR_BINARY_EQU, L"setz");
				TRANSLATE_BOOLEAN_OP(IR_BINARY_LESS, L"setl");
				TRANSLATE_BOOLEAN_OP(IR_BINARY_GREATER, L"setg");

			case IR_BRANCH_UNCONDITIONAL:
				swprintf(fmt_buf, FMT_BUF_MAX_LEN, L"jmp %ls", inst->operands[0].label);
				bufncpy(writer, fmt_buf);
				break;
			case IR_BRANCH_CONDITIONAL:
				// bufncpy(writer, L"; -- conditional jump");

				var_assignment_t cond_var = assignments[inst->operands[0].var_idx];
				character_t *jump_label = inst->operands[1].label;
				assign_registers(writer, &cond_var, false);

				swprintf(fmt_buf, FMT_BUF_MAX_LEN, L"cmp %ls, 1", cond_var.current_reg);
				bufncpy(writer, fmt_buf);

				swprintf(fmt_buf, FMT_BUF_MAX_LEN, L"je %ls", jump_label);
				bufncpy(writer, fmt_buf);

				// bufncpy(writer, L"; -- end conditional jump");
				break;

			case IR_LABEL:
				swprintf(fmt_buf, FMT_BUF_MAX_LEN, L"%ls:", inst->operands[0].label);
				bufncpy(writer, fmt_buf);
				break;
			default:
				// fprintf(stderr, "Unhandled opcode: %d\n", inst->type);
				break;
		}
		// printf("opcode=%d, op1=%d, op2=%d\n", inst->type, inst->operand1_idx, inst->operand2_idx);
	}

	// bufncpy(writer, L"; end function");
}

#define DEFAULT_ASM_ALLOC 1024

wchar_t *translate_ir_to_x86(const wchar_t *ir_buf)
{
	printf("translating to x86\n");
	ir_program_t prog = parse_ir(ir_buf);

	buf_writer_t writer = {writer.buf = (character_t *)calloc(DEFAULT_ASM_ALLOC, sizeof(character_t)),
			       .buf_len = DEFAULT_ASM_ALLOC};

	bufncpy(&writer, L"\%include \"std/std.inc\"");
	bufncpy(&writer, L"global _start");

	for (list_elem_t *fn_node = list_begin(&prog.functions); fn_node; fn_node = list_next(&prog.functions, fn_node))
	{
		ir_function_t *fn = fn_node->data;
		// printf("translating: %ls, args: %d varcnt: %d\n", fn->name, fn->arg_cnt, fn->var_cnt);
		translate_function(&writer, fn);
	}

	return writer.buf;
}
