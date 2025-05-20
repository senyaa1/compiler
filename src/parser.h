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

typedef struct ASTNode
{
	ast_node_type_t type;
	union {
		number_t number;
		character_t *identifier;
		struct
		{
			struct ASTNode *left;
			struct ASTNode *right;
		} assignment;
		struct
		{
			struct ASTNode *identifier;
			struct ASTNode *initializer;
		} declaration;
		struct
		{
			struct ASTNode *condition;
			struct ASTNode *then_branch;
			struct ASTNode *else_branch;
		} if_statement;
		struct
		{
			struct ASTNode *condition;
			struct ASTNode *body;
		} while_statement;
		struct
		{
			struct ASTNode *initializer;
			struct ASTNode *condition;
			struct ASTNode *iterator;
			struct ASTNode *body;
		} for_statement;
		struct
		{
			struct ASTNode *name;
			struct ASTNode **parameters;
			size_t param_count;
			struct ASTNode *body;
		} function;
		struct
		{
			struct ASTNode **children;
			size_t child_count;
		} block;
		struct
		{
			struct ASTNode **children;
			size_t child_count;
		} program;
		struct
		{
			struct ASTNode *left;
			struct ASTNode *right;
			token_type_t op;
		} binary_op;
		struct
		{
			struct ASTNode *name;
			struct ASTNode **arguments;
			size_t arg_count;
		} function_call;
		struct
		{
			struct ASTNode *value;
		} return_statement;
		struct
		{
			character_t *value;
		} string;
	} data;
} ASTNode;


ASTNode *parse_block();
ASTNode *parse_statement();
ASTNode *parse_expression();
ASTNode *parse_assignment();
ASTNode *parse_declaration();
ASTNode *parse_function();
ASTNode *parse_if();
ASTNode *parse_while();
ASTNode *parse_primary();
ASTNode *parse_math_expr();
ASTNode *parse_program(token_t *tokens);
void free_ast(ASTNode *node);
