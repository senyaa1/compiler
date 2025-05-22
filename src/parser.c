#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include "buffer.h"
#include "lexer.h"
#include "parser.h"

static token_t *tokens = 0;
static size_t token_index = 0;

static token_t *current_token()
{
	return &tokens[token_index];
}

static void advance_token()
{
	token_index++;
}

ast_node_t *create_ast_node(ast_node_type_t type)
{
	ast_node_t *node = (ast_node_t *)calloc(1, sizeof(ast_node_t));
	node->type = type;
	return node;
}

ast_node_t *parse_block()
{
	expect_token(TOKEN_LBRACE);
	ast_node_t *block_node = create_ast_node(AST_BLOCK);
	block_node->data.block.children = NULL;
	block_node->data.block.child_count = 0;

	while (current_token()->type != TOKEN_RBRACE)
	{
		ast_node_t *stmt = parse_statement();
		block_node->data.block.children = realloc(block_node->data.block.children,
							  sizeof(ast_node_t *) * (block_node->data.block.child_count + 1));
		block_node->data.block.children[block_node->data.block.child_count++] = stmt;
	}

	expect_token(TOKEN_RBRACE);
	return block_node;
}
ast_node_t *parse_function_call()
{
	printf("parsing function call!\n");
	ast_node_t *node = create_ast_node(AST_FUNCTION_CALL);

	// Parse function name
	node->data.function_call.name = create_ast_node(AST_IDENTIFIER);
	node->data.function_call.name->data.identifier = wcsdup(current_token()->value.str);
	expect_token(TOKEN_IDENTIFIER);

	// Parse argument list
	expect_token(TOKEN_LPAREN);
	node->data.function_call.arguments = NULL;
	node->data.function_call.arg_count = 0;

	while (current_token()->type != TOKEN_RPAREN)
	{
		ast_node_t *arg = parse_expression();
		node->data.function_call.arguments = realloc(
		    node->data.function_call.arguments, sizeof(ast_node_t *) * (node->data.function_call.arg_count + 1));
		node->data.function_call.arguments[node->data.function_call.arg_count++] = arg;

		if (current_token()->type == TOKEN_COMMA)
			advance_token();
		else
			break;
	}

	expect_token(TOKEN_RPAREN);
	return node;
}

ast_node_t *parse_asm()
{
	expect_token(TOKEN_KEYWORD); // asm
	ast_node_t *node = create_ast_node(AST_INLINE_ASM);

	// expect_token(TOKEN_STRING);
	if(current_token()->type != TOKEN_STRING)
	{	
		fprintf(stderr, "error: expected string token after asm keyword\n");
		exit(1);
	}

	node->data.string.value = current_token()->value.str;
	advance_token();

	expect_token(TOKEN_SEMICOLON);

	printf("parsed asm\n");

	return node;
}

int expect_token(token_type_t type)
{
	if (current_token()->type == type)
	{
		advance_token();
		return 1;
	}
	fprintf(stderr, "Unexpected token: %d, while was expecting %d\n", current_token()->type, type);
	exit(1);

	return 0;
}

ast_node_t *parse_return()
{
	expect_token(TOKEN_KEYWORD); // return
	ast_node_t *node = create_ast_node(AST_RETURN);

	node->data.return_statement.value = 0;
	if (current_token()->type != TOKEN_SEMICOLON)
		node->data.return_statement.value = parse_expression();

	expect_token(TOKEN_SEMICOLON);
	return node;
}

ast_node_t *parse_for()
{
	expect_token(TOKEN_KEYWORD); // for

	ast_node_t *node = create_ast_node(AST_FOR);
	expect_token(TOKEN_LPAREN);

	if (current_token()->type == TOKEN_KEYWORD && wcscmp(current_token()->value.str, L"цель") == 0)
		node->data.for_statement.initializer = parse_declaration();
	else if (current_token()->type != TOKEN_SEMICOLON)
		node->data.for_statement.initializer = parse_expression();

	expect_token(TOKEN_SEMICOLON);

	if (current_token()->type != TOKEN_SEMICOLON)
		node->data.for_statement.condition = parse_expression();

	expect_token(TOKEN_SEMICOLON);

	if (current_token()->type != TOKEN_RPAREN)
		node->data.for_statement.iterator = parse_expression();

	expect_token(TOKEN_RPAREN);

	node->data.for_statement.body = parse_block();
	return node;
}


ast_node_t *parse_statement()
{
#define PARSE_KEYWORD(KEYWORD, FUNC)                                                                                   \
	if (wcscmp(current_token()->value.str, WIDEN(#KEYWORD)) == 0)                                                  \
		FUNC;

	if (current_token()->type == TOKEN_KEYWORD)
	{
		if (wcscmp(current_token()->value.str, L"цель") == 0) // ; handled differently
		{
			ast_node_t *decl = parse_declaration();
			expect_token(TOKEN_SEMICOLON);
			return decl;
		}

		PARSE_KEYWORD(действо, return parse_function());
		PARSE_KEYWORD(аще, return parse_if());
		PARSE_KEYWORD(доколе, return parse_while());
		PARSE_KEYWORD(ради, return parse_for());
		PARSE_KEYWORD(съборъ, return parse_asm());
		PARSE_KEYWORD(воздать, return parse_return());
	}

	ast_node_t *expr = parse_expression();
	expect_token(TOKEN_SEMICOLON);
	return expr;


#undef PARSE_KEYWORD
}

ast_node_t *parse_declaration()
{
	expect_token(TOKEN_KEYWORD); // variable type ('int')

	ast_node_t *node = create_ast_node(AST_DECLARATION);
	node->data.declaration.identifier = create_ast_node(AST_IDENTIFIER);
	node->data.declaration.identifier->data.identifier = wcsdup(current_token()->value.str);
	expect_token(TOKEN_IDENTIFIER);

	if (current_token()->type == TOKEN_ASSIGN)
	{
		advance_token();
		printf("parsing expression to init var\n");
		node->data.declaration.initializer = parse_expression();
	}
	else
	{
		printf("skipping initializer\n");
		node->data.declaration.initializer = NULL;
	}

	// expect_token(TOKEN_SEMICOLON); don't consume here
	fprintf(stderr, "parsed declaration\n");
	return node;
}

ast_node_t *parse_function()
{
	expect_token(TOKEN_KEYWORD); // 'func'

	ast_node_t *node = create_ast_node(AST_FUNCTION);
	node->data.function.name = create_ast_node(AST_IDENTIFIER);
	node->data.function.name->data.identifier = wcsdup(current_token()->value.str);
	expect_token(TOKEN_IDENTIFIER);

	expect_token(TOKEN_LPAREN);
	node->data.function.parameters = NULL;
	node->data.function.param_count = 0;

	while (current_token()->type != TOKEN_RPAREN)
	{
		ast_node_t *param = parse_declaration();
		// advance_token();

		node->data.function.parameters =
		    realloc(node->data.function.parameters, sizeof(ast_node_t *) * (node->data.function.param_count + 1));
		node->data.function.parameters[node->data.function.param_count++] = param;

		if (current_token()->type == TOKEN_COMMA)
			advance_token();
		else
			break;
	}

	expect_token(TOKEN_RPAREN);
	node->data.function.body = parse_block();
	return node;
}

ast_node_t *parse_if()
{
	expect_token(TOKEN_KEYWORD); // 'if'

	ast_node_t *node = create_ast_node(AST_IF);
	expect_token(TOKEN_LPAREN);
	node->data.if_statement.condition = parse_expression_priority();
	expect_token(TOKEN_RPAREN);

	node->data.if_statement.then_branch = parse_block();

	if (current_token()->type == TOKEN_KEYWORD && wcscmp(current_token()->value.str, L"инако") == 0)
	{
		advance_token();
		node->data.if_statement.else_branch = parse_block();
	}
	else
	{
		node->data.if_statement.else_branch = NULL;
	}

	return node;
}

ast_node_t *parse_while()
{
	expect_token(TOKEN_KEYWORD); // 'while'

	ast_node_t *node = create_ast_node(AST_WHILE);
	expect_token(TOKEN_LPAREN);
	node->data.while_statement.condition = parse_expression();
	expect_token(TOKEN_RPAREN);

	node->data.while_statement.body = parse_block();
	return node;
}

ast_node_t *parse_expression()
{
	if (current_token()->type == TOKEN_IDENTIFIER && tokens[token_index + 1].type == TOKEN_ASSIGN)
	{
		printf("parsing assignment\n");
		return parse_assignment();
	}

	return parse_expression_priority();
}

ast_node_t *parse_assignment()
{
	ast_node_t *node = create_ast_node(AST_ASSIGNMENT);

	// left-hand side (identifier)
	node->data.assignment.left = create_ast_node(AST_IDENTIFIER);
	node->data.assignment.left->data.identifier = wcsdup(current_token()->value.str);
	expect_token(TOKEN_IDENTIFIER);

	expect_token(TOKEN_ASSIGN);

	// right-hand sand
	node->data.assignment.right = parse_expression_priority();

	fprintf(stderr, "parsed assignment\n");
	return node;
}
ast_node_t *parse_primary()
{
	if (current_token()->type == TOKEN_NUMBER)
	{
		ast_node_t *node = create_ast_node(AST_NUMBER);
		node->data.number = current_token()->value.number;
		advance_token();
		return node;
	}

	if (current_token()->type == TOKEN_IDENTIFIER)
	{
		// Lookahead to check if this is a function call
		if (tokens[token_index + 1].type == TOKEN_LPAREN)
		{
			return parse_function_call();
		}

		ast_node_t *node = create_ast_node(AST_IDENTIFIER);
		node->data.identifier = wcsdup(current_token()->value.str);
		advance_token();
		return node;
	}

	if (current_token()->type == TOKEN_LPAREN)
	{
		advance_token(); // Consume '('
		ast_node_t *expr = parse_expression_priority();
		if (current_token()->type != TOKEN_RPAREN)
		{
			fprintf(stderr, "Expected ')' after expression\n");
			return 0;
		}
		advance_token(); // Consume ')'
		return expr;
	}

	fprintf(stderr, "Unexpected token in primary: Type=%d, Value=%ls\n", current_token()->type,
		current_token()->type == TOKEN_KEYWORD || current_token()->type == TOKEN_IDENTIFIER
		    ? current_token()->value.str
		    : L"N/A");
	exit(1);
	return 0;
}

void free_ast(ast_node_t *node)
{
	if (!node)
		return;

	switch (node->type)
	{
		case AST_NUMBER:
			break;
		case AST_IDENTIFIER:
			free(node->data.identifier);
			break;
		case AST_ASSIGNMENT:
			free_ast(node->data.assignment.left);
			free_ast(node->data.assignment.right);
			break;
		case AST_DECLARATION:
			free_ast(node->data.declaration.identifier);
			free_ast(node->data.declaration.initializer);
			break;
		case AST_FUNCTION:
			free_ast(node->data.function.name);
			for (size_t i = 0; i < node->data.function.param_count; ++i)
			{
				free_ast(node->data.function.parameters[i]);
			}
			free(node->data.function.parameters);
			free_ast(node->data.function.body);
			break;
		case AST_IF:
			free_ast(node->data.if_statement.condition);
			free_ast(node->data.if_statement.then_branch);
			free_ast(node->data.if_statement.else_branch);
			break;
		case AST_WHILE:
			free_ast(node->data.while_statement.condition);
			free_ast(node->data.while_statement.body);
			break;
		case AST_BLOCK:
			for (size_t i = 0; i < node->data.block.child_count; ++i)
			{
				free_ast(node->data.block.children[i]);
			}
			free(node->data.block.children);
			break;
		case AST_PROGRAM:
			for (size_t i = 0; i < node->data.program.child_count; ++i)
			{
				free_ast(node->data.program.children[i]);
			}
			free(node->data.program.children);
			break;
		case AST_FOR:
			free_ast(node->data.for_statement.condition);
			free_ast(node->data.for_statement.body);
			free_ast(node->data.for_statement.initializer);
			free_ast(node->data.for_statement.iterator);
			break;
		case AST_INLINE_ASM:
			// free(node->data.identifier);
			break;
		case AST_STRING:
			free(node->data.string.value);
			break;
		default:
			break;
	}

	free(node);
}

ast_node_t *parse_program(token_t *t)
{
	tokens = t;
	token_index = 0;

	ast_node_t *program_node = create_ast_node(AST_PROGRAM);
	program_node->data.program.children = NULL;
	program_node->data.program.child_count = 0;

	while (current_token()->type != TOKEN_EOF)
	{
		ast_node_t *stmt = parse_statement();
		program_node->data.program.children =
		    realloc(program_node->data.program.children,
			    sizeof(ast_node_t *) * (program_node->data.program.child_count + 1));
		program_node->data.program.children[program_node->data.program.child_count++] = stmt;
	}

	return program_node;
}

static ast_node_t *create_binary_op_node(ast_node_t *left, token_type_t op, ast_node_t *right)
{
	ast_node_t *node = create_ast_node(AST_BINARY);
	node->data.binary_op.left = left;
	node->data.binary_op.op = op;
	node->data.binary_op.right = right;
	return node;
}

#define PARSE(NAME, CONDITION, NEXT)                                                                                   \
	ast_node_t *NAME()                                                                                                \
	{                                                                                                              \
		ast_node_t *node = NEXT();                                                                                \
		if (!node)                                                                                             \
			return 0;                                                                                      \
		while (CONDITION)                                                                                      \
		{                                                                                                      \
			token_type_t op = current_token()->type;                                                       \
			advance_token();                                                                               \
			ast_node_t *right = NEXT();                                                                       \
			node = create_binary_op_node(node, op, right);                                                 \
		}                                                                                                      \
		return node;                                                                                           \
	}


PARSE(parse_term, current_token()->type == TOKEN_STAR || current_token()->type == TOKEN_SLASH, parse_primary)

PARSE(parse_math_expr, current_token()->type == TOKEN_PLUS || current_token()->type == TOKEN_MINUS, parse_term)

PARSE(parse_logic_expr,
      current_token()->type == TOKEN_CARET || current_token()->type == TOKEN_AMPERSAND ||
	  current_token()->type == TOKEN_BAR,
      parse_math_expr)

PARSE(parse_expression_priority,
      current_token()->type == TOKEN_GREATER || current_token()->type == TOKEN_LESS ||
	  current_token()->type == TOKEN_EQUALS,
      parse_logic_expr)
