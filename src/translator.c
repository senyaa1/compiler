#include <stdio.h>
#include <wchar.h>

#include "buffer.h"
#include "lexer.h"
#include "parser.h"
#include "translator.h"


character_t *generate_label(character_t *prefix)
{
	static size_t identifier_cnt = 0;
	size_t prefix_len = wcslen(prefix);
	character_t *label = calloc(prefix_len + 32, sizeof(character_t));

	swprintf(label, prefix_len + 32, L"%ls_%zu", prefix, identifier_cnt++);

	return label;
}

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


static identifier_t *identifiers = 0;
static size_t identifier_cnt = 0;
static size_t identifiers_allocated = 0;


static const size_t DEFAULT_IDENTIFIER_ALLOC = 256;

static character_t *ENTRYPOINT_NAME = L"main";

void init_identifiers()
{
	if (identifiers)
		return;
	identifiers_allocated = DEFAULT_IDENTIFIER_ALLOC;
	identifiers = (identifier_t *)calloc(identifiers_allocated, sizeof(identifier_t));
}

void free_identifiers()
{
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

identifier_t *add_var(character_t *name, scope_t scope)
{
	ensure_allocated();
	character_t *label = generate_label(name);
	identifiers[identifier_cnt++] = (identifier_t){.type = TYPE_VARIABLE,
						       .identifier = wcsdup(name),
						       .value.variable.scope = scope,
						       .value.variable.label = label};

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

identifier_t *get_variable(character_t *name)
{
	for (int i = 0; i < identifier_cnt; i++)
	{
		if (identifiers[i].type != TYPE_VARIABLE || wcscmp(identifiers[i].identifier, name))
			continue;

		return &identifiers[i];
	}

	return 0;
}

int push_var(buf_writer_t *writer, character_t *identifier)
{
	if (!identifier)
	{
		printf("invalid identifier!\n");
		exit(1);
		return 0;
	}
	bufcpy(writer, L";(ident)  ");
	bufncpy(writer, identifier);
	identifier_t *var = get_variable(identifier);

	if (!var)
		return 0;

	bufcpy(writer, L"push [");
	bufcpy(writer, var->value.variable.label);
	bufncpy(writer, L"]");

	return 1;
}

void pop_var(buf_writer_t *writer, character_t *identifier)
{
	bufcpy(writer, L";(ident)  ");
	bufncpy(writer, identifier);
	identifier_t *var = get_variable(identifier);

	if (!var)
		return;

	bufncpy(writer, L"pop ax");

	bufcpy(writer, L"mov [");
	bufcpy(writer, var->value.variable.label);
	bufncpy(writer, L"], ax");
}

#define FMT_BUF_MAX_LEN 512

void translate_recursive(buf_writer_t *writer, ASTNode *node)
{
	if (!node)
		return;

	switch (node->type)
	{
		case AST_IDENTIFIER:
			push_var(writer, node->data.identifier);
			break;
		case AST_NUMBER:
			character_t fmtbuf[FMT_BUF_MAX_LEN] = { 0 };
			swprintf(fmtbuf, FMT_BUF_MAX_LEN, L"push %ld", node->data.number);
			bufncpy(writer, fmtbuf);
			break;
		case AST_BLOCK:
			for (int i = 0; i < node->data.block.child_count; i++)
				translate_recursive(writer, node->data.block.children[i]);
			break;
		case AST_PROGRAM:
			bufncpy(writer, L"; program begin");
			for (int i = 0; i < node->data.program.child_count; i++)
				translate_recursive(writer, node->data.program.children[i]);
			bufncpy(writer, L"; program end");
			break;
		case AST_DECLARATION:
			// assume global
			character_t *var_label =
			    add_var(node->data.declaration.identifier->data.identifier, GLOBAL)->value.variable.label;
			printf("declared var with name %ls\n", node->data.declaration.identifier->data.identifier);

			translate_recursive(writer, node->data.declaration.initializer);
			bufncpy(writer, L"pop ax");
			bufcpy(writer, L"mov [");
			bufcpy(writer, var_label);
			bufncpy(writer, L"], ax");
			break;
		case AST_FUNCTION:
			bufncpy(writer, L"\n");
			character_t *func_label =
			    add_function(node->data.function.name->data.identifier, node->data.function.param_count);
			bufcpy(writer, func_label);
			bufncpy(writer, L":");

			bufncpy(writer, L"; args");
			for (int i = 0; i < node->data.function.param_count; i++)
			{
				translate_recursive(writer, node->data.function.parameters[i]);
			}

			translate_recursive(writer, node->data.function.body);
			bufncpy(writer, L"\n");
			break;
		case AST_ASSIGNMENT:
			translate_recursive(writer, node->data.assignment.right);
			pop_var(writer, node->data.assignment.left->data.identifier);
			// bufncpy(writer, "; var");
			break;
		case AST_BINARY:
			if (!node->data.binary_op.op)
				break;

			translate_recursive(writer, node->data.binary_op.left);
			translate_recursive(writer, node->data.binary_op.right);

			switch (node->data.binary_op.op)
			{
				case TOKEN_PLUS:
					bufncpy(writer, L"add");
					break;
				case TOKEN_MINUS:
					bufncpy(writer, L"sub");
					break;
				case TOKEN_STAR:
					bufncpy(writer, L"mul");
					break;
				case TOKEN_SLASH:
					bufncpy(writer, L"div");
					break;
				case TOKEN_CARET:
					bufncpy(writer, L"xor");
					break;
				case TOKEN_BAR:
					bufncpy(writer, L"or");
					break;
				case TOKEN_AMPERSAND:
					bufncpy(writer, L"and");
					break;
				case TOKEN_EQUALS:
					bufncpy(writer, L"cmp");
					bufncpy(writer, L"push FLAGS");
					bufncpy(writer, L"push 1");
					bufncpy(writer, L"and");
					bufncpy(writer, L"push 0");
					bufncpy(writer, L"cmp");
					break;
				case TOKEN_LESS:
					bufncpy(writer, L"cmp");
					bufncpy(writer, L"push FLAGS");
					bufncpy(writer, L"push 2");
					bufncpy(writer, L"and");
					bufncpy(writer, L"push 0");
					bufncpy(writer, L"cmp");
					break;
				case TOKEN_GREATER:
					bufncpy(writer, L"cmp");
					bufncpy(writer, L"push FLAGS");
					bufncpy(writer, L"push 2");
					bufncpy(writer, L"and");
					bufncpy(writer, L"push 2");
					bufncpy(writer, L"xor");
					bufncpy(writer, L"push 0");
					bufncpy(writer, L"cmp");
					break;
				default:
					break;
			}
			break;
		case AST_IF:
			character_t *if_label = generate_label(L"if");
			character_t *else_label = generate_label(L"else");

			bufncpy(writer, L"; begin if");

			translate_recursive(writer, node->data.if_statement.condition);
			bufcpy(writer, L"jle ");
			bufncpy(writer, else_label);

			translate_recursive(writer, node->data.if_statement.then_branch);
			if (node->data.if_statement.else_branch)
			{
				bufcpy(writer, L"jmp ");
				bufncpy(writer, if_label);
			}

			bufcpy(writer, else_label);
			bufncpy(writer, L":");

			if (node->data.if_statement.else_branch)
			{
				bufncpy(writer, L"; else");
				translate_recursive(writer, node->data.if_statement.else_branch);
				bufcpy(writer, if_label);
				bufncpy(writer, L":");
			}

			break;

		case AST_WHILE:
			character_t *end_while = generate_label(L"whileend");
			character_t *start_while = generate_label(L"whilestart");

			bufcpy(writer, start_while);
			bufncpy(writer, L":");

			translate_recursive(writer, node->data.while_statement.condition);
			bufcpy(writer, L"jle ");
			bufncpy(writer, end_while);

			bufncpy(writer, L"; body");
			translate_recursive(writer, node->data.while_statement.body);
			bufcpy(writer, L"jmp ");
			bufncpy(writer, start_while);

			bufcpy(writer, end_while);
			bufncpy(writer, L":");

			break;
		case AST_INLINE_ASM:
			bufncpy(writer, L"; inline asm");
			bufncpy(writer, node->data.string.value);
			bufncpy(writer, L"; inline asm end");
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

			for (int i = 0; i < node->data.function_call.arg_count; i++)
			{
				translate_recursive(writer, node->data.function_call.arguments[i]);
				// if(!push_var(writer, node->data.function_call.arguments[i]->data.identifier))
				// if(node->data.function_call.arguments[i]->type == TYPE_VARIABLE && push_var(writer,
				// node->data.function_call.arguments[i]->data.identifier))
				// {
				// 	fprintf(stderr, "Unknown argument in function %s: %s\n",
				// node->data.function_call.name->data.identifier,
				// node->data.function_call.arguments[i]->data.identifier); 	exit(1);
				// }
				// else
				// {
				// 	bufcpy(writer, "push ");
				// }
			}

			bufcpy(writer, L"call ");
			bufncpy(writer, called->value.function.label);
			bufncpy(writer, L"push dx");
			break;
		case AST_RETURN:
			translate_recursive(writer, node->data.return_statement.value);
			bufncpy(writer, L"pop dx");
			bufncpy(writer, L"ret");
			break;
		case AST_FOR:
			character_t *end_for = generate_label(L"for_end");
			character_t *start_for = generate_label(L"for_start");

			character_t *init_for = generate_label(L"for_init");
			bufcpy(writer, init_for);
			bufncpy(writer, L":");
			translate_recursive(writer, node->data.for_statement.initializer);


			bufcpy(writer, start_for);
			bufncpy(writer, L":");

			translate_recursive(writer, node->data.for_statement.condition);
			bufcpy(writer, L"jle ");
			bufncpy(writer, end_for);

			bufncpy(writer, L"; body");
			translate_recursive(writer, node->data.for_statement.body);

			bufncpy(writer, L"; iterator");
			translate_recursive(writer, node->data.for_statement.iterator);

			bufcpy(writer, L"jmp ");
			bufncpy(writer, start_for);

			bufcpy(writer, end_for);
			bufncpy(writer, L":");

			break;
			break;
		default:
			fprintf(stderr, "unknown node! (%d)\n", node->type);
			exit(1);
			break;
	}
}


buf_writer_t translate(ASTNode *ast)
{
	buf_writer_t writer = {writer.buf = (character_t *)calloc(DEFAULT_ASM_ALLOC, sizeof(character_t)),
			       .buf_len = DEFAULT_ASM_ALLOC};

	// bufncpy(&writer, L"call main");
	// bufncpy(&writer, L"hlt");

	translate_recursive(&writer, ast);

	bufncpy(&writer, L"; data");

	for (int i = 0; i < identifier_cnt; i++)
	{
		if (identifiers[i].type != TYPE_VARIABLE)
			continue;

		bufcpy(&writer, identifiers[i].value.variable.label);
		bufncpy(&writer, L":");
		bufncpy(&writer, L"\t dq 0");
	}
	bufend(&writer);

	free_identifiers();

	return writer;
}
