#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>


#include "buffer.h"
#include "ir.h"
#include "lexer.h"
#include "parser.h"

/* FIX:
 *	Variable scope
 */

static identifier_t *identifiers = 0;
static size_t identifier_cnt = 0;
static size_t identifiers_allocated = 0;

static translator_ctx_t ctx = {};

character_t *generate_label(character_t *prefix)
{
	static size_t identifier_cnt = 0;
	size_t prefix_len = wcslen(prefix);
	character_t *label = calloc(prefix_len + 32, sizeof(character_t));

	swprintf(label, prefix_len + 32, L"%ls_%zu", prefix, identifier_cnt++);

	return label;
}

void init_identifiers()
{
	if (identifiers)
		return;
	identifiers_allocated = DEFAULT_IDENTIFIER_ALLOC;
	identifiers = (identifier_t *)calloc(identifiers_allocated, sizeof(identifier_t));
}

void free_identifiers()
{
	for(int i = 0; i < identifier_cnt; i++)
	{
		free(identifiers[i].identifier);
		free(identifiers[i].function_name);
	}
	free(identifiers);
}

void ensure_allocated()
{
	if (!identifiers)
		init_identifiers();
	if (identifier_cnt + 1 >= identifiers_allocated)
	{
		identifiers_allocated *= 2;
		identifiers = (identifier_t *)realloc(identifiers, identifiers_allocated * sizeof(identifier_t));
	}
}

identifier_t *add_var(character_t *name, scope_t scope, int idx, character_t* func_name)
{
	ensure_allocated();
	character_t *label = generate_label(name);
	identifiers[identifier_cnt++] = (identifier_t){.type = TYPE_VARIABLE,
						       .identifier = wcsdup(name),
						       .function_name = wcsdup(func_name),
						       .value.variable.scope = scope,
						       .value.variable.label = label,
						       .value.variable.var_idx = idx};

	return &identifiers[identifier_cnt - 1];
}

character_t *add_function(character_t *name, size_t arg_cnt)
{
	ensure_allocated();
	character_t *label = (wcscmp(name, ENTRYPOINT_NAME) == 0) ? wcsdup(name) : generate_label(name);

	identifiers[identifier_cnt++] = (identifier_t){.type = TYPE_FUNCTION,
						       .identifier = wcsdup(name),
						       .value.function.label = label,
						       .value.function.arg_cnt = arg_cnt};

	fprintf(stderr, "adding function named %ls with %ld arguments with label %ls\n", name, arg_cnt, label);
	return label;
}

identifier_t *get_function(character_t *name)
{
	for (int i = 0; i < identifier_cnt; i++)
	{
		if (identifiers[i].type != TYPE_FUNCTION || wcscmp(identifiers[i].identifier, name))
			continue;

		return &identifiers[i];
	}

	return 0;
}

identifier_t *get_variable(character_t *name, character_t* func_scope)
{
	for (int i = 0; i < identifier_cnt; i++)
	{
		if (identifiers[i].type != TYPE_VARIABLE || wcscmp(identifiers[i].identifier, name) || wcscmp(identifiers[i].function_name, func_scope))
			continue;

		return &identifiers[i];
	}

	return 0;
}

int translate_recursive(buf_writer_t *writer, ast_node_t *node)
{
	if (!node)
		return -1;

	character_t line_buf[LINE_BUF_SZ] = {0};

	switch (node->type)
	{
		case AST_IDENTIFIER:
			identifier_t *ident = get_variable(node->data.identifier, ctx.current_function);
			if (!ident)
			{
				fprintf(stderr, "unknown identifier: %ls\n", node->data.identifier);
				exit(1);
			}
			return ident->value.variable.var_idx;
		case AST_NUMBER:
			int const_idx = ++ctx.scope_var_cnt;
			swprintf(line_buf, LINE_BUF_SZ, L"%%%d = const %d", const_idx, node->data.number);
			bufncpy(writer, line_buf);
			return const_idx;
		case AST_BLOCK:
			for (int i = 0; i < node->data.block.child_count; i++)
				translate_recursive(writer, node->data.block.children[i]);
			break;
		case AST_PROGRAM:
			for (int i = 0; i < node->data.program.child_count; i++)
				translate_recursive(writer, node->data.program.children[i]);
			break;
		case AST_DECLARATION:
			int initializer_idx = translate_recursive(writer, node->data.declaration.initializer);

			// int vardecl_idx = initializer_idx;

			int vardecl_idx = ++ctx.scope_var_cnt;
			// swprintf(line_buf, LINE_BUF_SZ, L"%%%d = vardecl %%%d", vardecl_idx, initializer_idx);
			swprintf(line_buf, LINE_BUF_SZ, L"%%%d = assign %%%d", vardecl_idx, initializer_idx);
			bufncpy(writer, line_buf);

			character_t *var_label =
			    add_var(node->data.declaration.identifier->data.identifier, LOCAL, vardecl_idx, ctx.current_function)
				->value.variable.label;
			printf("declared var with name %ls and local id %d\n",
			       node->data.declaration.identifier->data.identifier, vardecl_idx);
			return -1;
		case AST_FUNCTION:
			if (ctx.current_function)
			{
				fprintf(stderr, "can't define function within a function!\n");
				exit(1);
			}

			bufcpy(writer, L"function ");
			character_t *func_label =
			    add_function(node->data.function.name->data.identifier, node->data.function.param_count);

			bufcpy(writer, func_label);
			bufcpy(writer, L" (");

			ctx.current_function = func_label;
			ctx.scope_var_cnt = 0;

			for (int i = 0; i < node->data.function.param_count; i++)
			{
				if (i > 0)
					bufcpy(writer, L", ");

				character_t *arg_ident =
				    node->data.function.parameters[i]->data.declaration.identifier->data.identifier;

				int func_arg_idx = ++ctx.scope_var_cnt;
				swprintf(line_buf, LINE_BUF_SZ, L"int %%%d", func_arg_idx);
				bufcpy(writer, line_buf);

				add_var(arg_ident, ARG, func_arg_idx, ctx.current_function);

				printf("declared arg with name %ls and local id %d\n", arg_ident, func_arg_idx);
			}

			bufncpy(writer, L")\n{");
			translate_recursive(writer, node->data.function.body);

			swprintf(line_buf, LINE_BUF_SZ, L"} %d", ctx.scope_var_cnt);
			bufncpy(writer, line_buf);

			ctx.scope_var_cnt = 0;
			ctx.current_function = 0;
			break;
		case AST_ASSIGNMENT:
			int l_idx = translate_recursive(writer, node->data.assignment.left);
			int r_idx = translate_recursive(writer, node->data.assignment.right);

			swprintf(line_buf, LINE_BUF_SZ, L"%%%d = assign %%%d", l_idx, r_idx);
			bufncpy(writer, line_buf);

			return l_idx;
		case AST_BINARY:
			if (!node->data.binary_op.op)
				break;

			int left_idx = translate_recursive(writer, node->data.binary_op.left);
			int right_idx = translate_recursive(writer, node->data.binary_op.right);


			swprintf(line_buf, LINE_BUF_SZ, L"%%%d = ", ++ctx.scope_var_cnt);
			bufcpy(writer, line_buf);

			switch (node->data.binary_op.op)
			{
				case TOKEN_PLUS:
					swprintf(line_buf, LINE_BUF_SZ, L"add %%%d, %%%d", left_idx, right_idx);
					break;
				case TOKEN_MINUS:
					swprintf(line_buf, LINE_BUF_SZ, L"sub %%%d, %%%d", left_idx, right_idx);
					break;
				case TOKEN_STAR:
					swprintf(line_buf, LINE_BUF_SZ, L"mul %%%d, %%%d", left_idx, right_idx);
					break;
				case TOKEN_SLASH:
					swprintf(line_buf, LINE_BUF_SZ, L"div %%%d, %%%d", left_idx, right_idx);
					break;
				case TOKEN_CARET:
					swprintf(line_buf, LINE_BUF_SZ, L"xor %%%d, %%%d", left_idx, right_idx);
					break;
				case TOKEN_BAR:
					swprintf(line_buf, LINE_BUF_SZ, L"or %%%d, %%%d", left_idx, right_idx);
					break;
				case TOKEN_AMPERSAND:
					swprintf(line_buf, LINE_BUF_SZ, L"and %%%d, %%%d", left_idx, right_idx);
					break;
				case TOKEN_EQUALS:
					swprintf(line_buf, LINE_BUF_SZ, L"equ %%%d, %%%d", left_idx, right_idx);
					break;
				case TOKEN_LESS:
					swprintf(line_buf, LINE_BUF_SZ, L"lt %%%d, %%%d", left_idx, right_idx);
					break;
				case TOKEN_GREATER:
					swprintf(line_buf, LINE_BUF_SZ, L"gt %%%d, %%%d", left_idx, right_idx);
					break;
			}

			bufncpy(writer, line_buf);
			return ctx.scope_var_cnt;
		case AST_INLINE_ASM:
			bufcpy(writer, L"native_asm \"");
			bufcpy(writer, node->data.string.value);
			bufncpy(writer, L"\"");
			break;
		case AST_FUNCTION_CALL:
			identifier_t *called = get_function(node->data.function_call.name->data.identifier);

			if (!called)
			{
				fprintf(stderr, "Unknown function identifier %ls!\n",
					node->data.function_call.name->data.identifier);
				exit(1);
			}

			if (node->data.function_call.arg_count != called->value.function.arg_cnt)
			{
				fprintf(stderr, "Argument count mismatch in %ls, expected %ld got %ld\n",
					node->data.function_call.name->data.identifier, called->value.function.arg_cnt,
					node->data.function_call.arg_count);
				exit(1);
			}

			int *arg_idxs = calloc(node->data.function_call.arg_count, sizeof(int));

			for (int i = 0; i < node->data.function_call.arg_count; i++)
			{
				int arg_idx = translate_recursive(writer, node->data.function_call.arguments[i]);
				arg_idxs[i] = arg_idx;
				swprintf(line_buf, LINE_BUF_SZ, L"%%%d = %%%d", ++ctx.scope_var_cnt, arg_idx);
				bufncpy(writer, line_buf);
			}


			swprintf(line_buf, LINE_BUF_SZ, L"%%%d = call %ls %d", ++ctx.scope_var_cnt,
				 called->value.function.label, node->data.function_call.arg_count);
			bufcpy(writer, line_buf);

			for (int i = 0; i < node->data.function_call.arg_count; i++)
			{
				swprintf(line_buf, LINE_BUF_SZ, L" %%%d", arg_idxs[i]);
				bufcpy(writer, line_buf);
			}
			bufncpy(writer, L"");

			free(arg_idxs);
			return ctx.scope_var_cnt;
		case AST_RETURN:
			int retval_idx = translate_recursive(writer, node->data.return_statement.value);
			swprintf(line_buf, LINE_BUF_SZ, L"ret %%%d", retval_idx);
			bufncpy(writer, line_buf);
			break;
		case AST_FOR:
			character_t *end_for = generate_label(L"forend");
			character_t *start_for = generate_label(L"forstart");
			character_t *body_for = generate_label(L"forbody");

			translate_recursive(writer, node->data.for_statement.initializer);

			bufcpy(writer, start_for);
			bufncpy(writer, L":");

			int for_cond_idx = translate_recursive(writer, node->data.for_statement.condition);

			swprintf(line_buf, LINE_BUF_SZ, L"bc %%%d, %ls", for_cond_idx, body_for);
			bufncpy(writer, line_buf);

			bufcpy(writer, L"br ");
			bufncpy(writer, end_for);

			bufcpy(writer, body_for);
			bufncpy(writer, L":");

			translate_recursive(writer, node->data.for_statement.body);

			translate_recursive(writer, node->data.for_statement.iterator);

			bufcpy(writer, L"br ");
			bufncpy(writer, start_for);

			bufcpy(writer, end_for);
			bufncpy(writer, L":");
			break;
		case AST_IF:
			character_t *if_label = generate_label(L"if");
			character_t *if_end = generate_label(L"if_end");

			// bufncpy(writer, L"; begin if");

			int cond_idx = translate_recursive(writer, node->data.if_statement.condition);
			swprintf(line_buf, LINE_BUF_SZ, L"bc %%%d, %ls", cond_idx, if_label);
			bufncpy(writer, line_buf);

			if (node->data.if_statement.else_branch)
				translate_recursive(writer, node->data.if_statement.else_branch);

			bufcpy(writer, L"br ");
			bufncpy(writer, if_end);


			bufcpy(writer, if_label);
			bufncpy(writer, L":");

			translate_recursive(writer, node->data.if_statement.then_branch);

			bufcpy(writer, if_end);
			bufncpy(writer, L":");
			break;

		case AST_WHILE:
			character_t *end_while = generate_label(L"whileend");
			character_t *start_while = generate_label(L"whilestart");
			character_t *body_while = generate_label(L"whilebody");

			bufcpy(writer, start_while);
			bufncpy(writer, L":");

			int while_cond_idx = translate_recursive(writer, node->data.while_statement.condition);

			swprintf(line_buf, LINE_BUF_SZ, L"bc %%%d, %ls", while_cond_idx, body_while);
			bufncpy(writer, line_buf);

			bufcpy(writer, L"br ");
			bufncpy(writer, end_while);

			bufcpy(writer, body_while);
			bufncpy(writer, L":");

			translate_recursive(writer, node->data.while_statement.body);

			bufcpy(writer, L"br ");
			bufncpy(writer, start_while);

			bufcpy(writer, end_while);
			bufncpy(writer, L":");

			break;
			break;
		default:
			fprintf(stderr, "unknown node! (%d)\n", node->type);
			exit(1);
			break;
	}

	return -1;
}


wchar_t *translate_to_ir(ast_node_t *ast)
{
	buf_writer_t writer = {writer.buf = (character_t *)calloc(DEFAULT_ASM_ALLOC, sizeof(character_t)),
			       .buf_len = DEFAULT_ASM_ALLOC};

	translate_recursive(&writer, ast);
	bufend(&writer);

	free_identifiers();

	return writer.buf;
}


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
