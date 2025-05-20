#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "translator.h"
#include "buffer.h"


char* generate_label(char* prefix)
{
	static size_t identifier_cnt = 0;

	char* label = 0;
	asprintf(&label, "%ld_%s", identifier_cnt++, prefix);
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
	char* label;
	size_t arg_cnt;
} function_t;

typedef struct variable 
{
	scope_t scope;
	char* label;
} variable_t;

typedef struct identifier
{
	identifier_type_t type;
	character_t* identifier;

	union {
		function_t function;
		variable_t variable;
	} value;
} identifier_t;


static identifier_t* identifiers = 0;
static size_t identifier_cnt = 0;
static size_t identifiers_allocated = 0;


static const size_t DEFAULT_IDENTIFIER_ALLOC = 256;

static char* ENTRYPOINT_NAME = "main";

void init_identifiers()
{
	if(identifiers) return;
	identifiers_allocated = DEFAULT_IDENTIFIER_ALLOC;
	identifiers = (identifier_t*)calloc(identifiers_allocated, sizeof(identifier_t));
}

void free_identifiers()
{
	free(identifiers);
}

void ensure_allocated()
{
	if(!identifiers) init_identifiers();
	if(identifier_cnt + 1 >= identifiers_allocated)
	{
		identifiers_allocated *= 2;
		identifiers = (identifier_t*)realloc(identifiers, identifiers_allocated * sizeof(identifier_t));
	}
}

identifier_t* add_var(character_t* name, scope_t scope)
{	
	ensure_allocated();
	char* label = generate_label(name);
	identifiers[identifier_cnt++] = (identifier_t){.type = TYPE_VARIABLE, .identifier = strdup(name), .value.variable.scope = scope, .value.variable.label = label};

	return &identifiers[identifier_cnt - 1];
}

char* add_function(character_t* name, size_t arg_cnt)
{	
	ensure_allocated();
	char* label = (strcmp(name, ENTRYPOINT_NAME) == 0) ? strdup(name) : generate_label(name);

	identifiers[identifier_cnt++] = (identifier_t){.type = TYPE_FUNCTION, .identifier = strdup(name), .value.function.label = label, .value.function.arg_cnt = arg_cnt};

	fprintf(stderr, "adding function named %s with %ld arguments with label %s\n", name, arg_cnt, label);
	return label;
}

identifier_t* get_function(character_t* name)
{
	for(int i = 0; i < identifier_cnt; i++)
	{
		if(identifiers[i].type != TYPE_FUNCTION || strcmp(identifiers[i].identifier, name))
			continue;

		return &identifiers[i];
	}

	return 0;
}

identifier_t* get_variable(character_t* name)
{
	for(int i = 0; i < identifier_cnt; i++)
	{
		if(identifiers[i].type != TYPE_VARIABLE || strcmp(identifiers[i].identifier, name))
			continue;

		return &identifiers[i];
	}

	return 0;
}

int push_var(buf_writer_t* writer, character_t* identifier)
{
	if(!identifier)
	{
		printf("invalid identifier!\n");
		exit(1);
		return 0;
	}
	bufcpy(writer, ";(ident)  ");
	bufncpy(writer, identifier);
	identifier_t* var = get_variable(identifier);

	if(!var)	
		return 0;

	bufcpy(writer, "push [");
	bufcpy(writer, var->value.variable.label);
	bufncpy(writer, "]");

	return 1;
}

void pop_var(buf_writer_t* writer, character_t* identifier)
{
	bufcpy(writer, ";(ident)  ");
	bufncpy(writer, identifier);
	identifier_t* var = get_variable(identifier);

	if(!var)	
		return;

	bufncpy(writer, "pop ax");

	bufcpy(writer, "mov [");
	bufcpy(writer, var->value.variable.label);
	bufncpy(writer, "], ax");
}

void translate_recursive(buf_writer_t* writer, ASTNode* node)
{
	if(!node) return;
	char* fmtbuf = 0;

	switch(node->type)
	{
		case AST_IDENTIFIER:
			push_var(writer, node->data.identifier);
			break;
		case AST_NUMBER:
			asprintf(&fmtbuf, "push %ld", node->data.number);
			bufncpy(writer, fmtbuf);
			break;
		case AST_BLOCK:
			for(int i = 0; i < node->data.block.child_count; i++)
				translate_recursive(writer, node->data.block.children[i]);
			break;
		case AST_PROGRAM:
			bufncpy(writer, "; program begin");
			for(int i = 0; i < node->data.program.child_count; i++)
				translate_recursive(writer, node->data.program.children[i]);
			bufncpy(writer, "; program end");
			break;
		case AST_DECLARATION:
			// assume global
			char* var_label = add_var(node->data.declaration.identifier->data.identifier, GLOBAL)->value.variable.label;
			printf("declared var with name %s\n", node->data.declaration.identifier->data.identifier);
			translate_recursive(writer, node->data.declaration.initializer);
			bufncpy(writer, "pop ax");
			bufcpy(writer, "mov [");
			bufcpy(writer, var_label);
			bufncpy(writer, "], ax");
			break;
		case AST_FUNCTION:
			bufncpy(writer, "\n");
			char* func_label = add_function(node->data.function.name->data.identifier, node->data.function.param_count);
			bufcpy(writer, func_label);
			bufncpy(writer, ":");

			bufncpy(writer, "; args");
			for(int i = 0; i < node->data.function.param_count; i++)
			{
				translate_recursive(writer, node->data.function.parameters[i]);
			}

			translate_recursive(writer, node->data.function.body);
			bufncpy(writer, "\n");
			break;
		case AST_ASSIGNMENT:
			translate_recursive(writer, node->data.assignment.right);
			pop_var(writer, node->data.assignment.left->data.identifier);
			// bufncpy(writer, "; var");
			break;
		case AST_BINARY:
			if(!node->data.binary_op.op)
				break;

			translate_recursive(writer, node->data.binary_op.left);
			translate_recursive(writer, node->data.binary_op.right);

			switch(node->data.binary_op.op)
			{
				case TOKEN_PLUS:
					bufncpy(writer, "add");
					break;
				case TOKEN_MINUS:
					bufncpy(writer, "sub");
					break;
				case TOKEN_STAR:
					bufncpy(writer, "mul");
					break;
				case TOKEN_SLASH:
					bufncpy(writer, "div");
					break;
				case TOKEN_CARET:
					bufncpy(writer, "xor");
					break;
				case TOKEN_BAR:
					bufncpy(writer, "or");
					break;
				case TOKEN_AMPERSAND:
					bufncpy(writer, "and");
					break;
				case TOKEN_EQUALS:
					bufncpy(writer, "cmp");
					bufncpy(writer, "push FLAGS");
					bufncpy(writer, "push 1");
					bufncpy(writer, "and");
					bufncpy(writer, "push 0");
					bufncpy(writer, "cmp");
					break;
				case TOKEN_LESS:
					bufncpy(writer, "cmp");
					bufncpy(writer, "push FLAGS");
					bufncpy(writer, "push 2");
					bufncpy(writer, "and");
					bufncpy(writer, "push 0");
					bufncpy(writer, "cmp");
					break;
				case TOKEN_GREATER:
					bufncpy(writer, "cmp");
					bufncpy(writer, "push FLAGS");
					bufncpy(writer, "push 2");
					bufncpy(writer, "and");
					bufncpy(writer, "push 2");
					bufncpy(writer, "xor");
					bufncpy(writer, "push 0");
					bufncpy(writer, "cmp");
					break;
				default:
					break;
			}
			break;
		case AST_IF:
			char* if_label = generate_label("if");
			char* else_label = generate_label("else");

			bufncpy(writer, "; begin if");

			translate_recursive(writer, node->data.if_statement.condition);
			bufcpy(writer, "jle ");
			bufncpy(writer, else_label);

			translate_recursive(writer, node->data.if_statement.then_branch);
			if(node->data.if_statement.else_branch)
			{
				bufcpy(writer, "jmp ");
				bufncpy(writer, if_label);
			}

			bufcpy(writer, else_label);
			bufncpy(writer, ":");

			if(node->data.if_statement.else_branch)
			{
				bufncpy(writer, "; else");
				translate_recursive(writer, node->data.if_statement.else_branch);
				bufcpy(writer, if_label);
				bufncpy(writer, ":");
			}

			break;

		case AST_WHILE:
			char* end_while = generate_label("whileend");
			char* start_while = generate_label("whilestart");

			bufcpy(writer, start_while);
			bufncpy(writer, ":");

			translate_recursive(writer, node->data.while_statement.condition);
			bufcpy(writer, "jle ");
			bufncpy(writer, end_while);

			bufncpy(writer, "; body");
			translate_recursive(writer, node->data.while_statement.body);
			bufcpy(writer, "jmp ");
			bufncpy(writer, start_while);

			bufcpy(writer, end_while);
			bufncpy(writer, ":");

			break;
		case AST_INLINE_ASM:
			bufncpy(writer, node->data.identifier);
			break;
		case AST_FUNCTION_CALL:
			identifier_t* called = get_function(node->data.function_call.name->data.identifier);

			if(!called)
			{
				fprintf(stderr, "Unknown function identifier %s!\n", node->data.function_call.name->data.identifier);
				exit(1);
			}

			if(node->data.function_call.arg_count != called->value.function.arg_cnt)
			{
				fprintf(stderr, "Argument count mismatch in %s, expected %ld got %ld\n", node->data.function_call.name->data.identifier, called->value.function.arg_cnt, node->data.function_call.arg_count);
				exit(1);
			}

			for(int i = 0; i < node->data.function_call.arg_count; i++)
			{
				translate_recursive(writer, node->data.function_call.arguments[i]);
				// if(!push_var(writer, node->data.function_call.arguments[i]->data.identifier))
				// if(node->data.function_call.arguments[i]->type == TYPE_VARIABLE && push_var(writer, node->data.function_call.arguments[i]->data.identifier))
				// {
				// 	fprintf(stderr, "Unknown argument in function %s: %s\n", node->data.function_call.name->data.identifier, node->data.function_call.arguments[i]->data.identifier);
				// 	exit(1);
				// }
				// else 
				// {
				// 	bufcpy(writer, "push ");
				// }

			}

			bufcpy(writer, "call ");
			bufncpy(writer, called->value.function.label);
			bufncpy(writer, "push dx");
			break;
		case AST_RETURN:
			translate_recursive(writer, node->data.return_statement.value);
			bufncpy(writer, "pop dx");
			bufncpy(writer, "ret");
			break;
		default:
			fprintf(stderr, "unknown node! (%d)\n", node->type);
			exit(1);
			break;
	}
}


buf_writer_t translate(ASTNode* ast)
{
	buf_writer_t writer = { writer.buf = (char*)calloc(DEFAULT_ASM_ALLOC, sizeof(char)), .buf_len = DEFAULT_ASM_ALLOC};

	bufncpy(&writer, "call main");
	bufncpy(&writer, "hlt");

	translate_recursive(&writer, ast);

	bufncpy(&writer, "; data");
	
	for(int i = 0; i < identifier_cnt; i++)
	{
		if(identifiers[i].type != TYPE_VARIABLE) continue;
		bufcpy(&writer, identifiers[i].value.variable.label);
		bufncpy(&writer, ":");
		bufncpy(&writer, "\t dq 0");
	}

	bufend(&writer);

	free_identifiers();

	return writer;
}


