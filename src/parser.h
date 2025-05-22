#pragma once

#include <stdint.h>

#include "lexer.h"

static const int DEFAULT_INLINE_ASM_ALLOC = 128;

typedef enum AST_NODE_TYPE
{
	AST_NUMBER,
	AST_IDENTIFIER,
	AST_ASSIGNMENT,
	AST_DECLARATION,
	AST_FUNCTION,
	AST_IF,
	AST_WHILE,
	AST_BLOCK,
	AST_PROGRAM,
	AST_BINARY,
	AST_FUNCTION_CALL,
	AST_INLINE_ASM,
	AST_RETURN,
	AST_FOR,
	AST_STRING,
} ast_node_type_t;

typedef struct ast_node ast_node_t;

struct ast_node
{
	ast_node_type_t type;
	union {
		number_t number;
		character_t *identifier;
		struct
		{
			ast_node_t *left;
			ast_node_t *right;
		} assignment;
		struct
		{
			ast_node_t *identifier;
			ast_node_t *initializer;
		} declaration;
		struct
		{
			ast_node_t *condition;
			ast_node_t *then_branch;
			ast_node_t *else_branch;
		} if_statement;
		struct
		{
			ast_node_t *condition;
			ast_node_t *body;
		} while_statement;
		struct
		{
			ast_node_t *initializer;
			ast_node_t *condition;
			ast_node_t *iterator;
			ast_node_t *body;
		} for_statement;
		struct
		{
			ast_node_t *name;
			ast_node_t **parameters;
			size_t param_count;
			ast_node_t *body;
		} function;
		struct
		{
			ast_node_t **children;
			size_t child_count;
		} block;
		struct
		{
			ast_node_t **children;
			size_t child_count;
		} program;
		struct
		{
			ast_node_t *left;
			ast_node_t *right;
			token_type_t op;
		} binary_op;
		struct
		{
			ast_node_t *name;
			ast_node_t **arguments;
			size_t arg_count;
		} function_call;
		struct
		{
			ast_node_t *value;
		} return_statement;
		struct
		{
			character_t *value;
		} string;
	} data;
};


ast_node_t *parse_expression_priority();
int expect_token(token_type_t type);

ast_node_t *parse_block();
ast_node_t *parse_statement();
ast_node_t *parse_expression();
ast_node_t *parse_assignment();
ast_node_t *parse_declaration();
ast_node_t *parse_function();
ast_node_t *parse_if();
ast_node_t *parse_while();
ast_node_t *parse_primary();
ast_node_t *parse_math_expr();
ast_node_t *parse_program(token_t *tokens);
void free_ast(ast_node_t *node);
