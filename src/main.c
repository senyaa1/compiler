#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "graph.h"
#include "fs.h"

/*	TODO:
 *	frontend
 *		build ast
 *	middleend	// last 
 *		optimize math expressions
 *		forcefully differentiate all expressions
 *	backend
 *		translator
 */

/*	syntax example
	
	ять-крест-крест (ykk)
 
	*/



int main(int argc, char** argv)
{
	// char* source_text = 0;
	// size_t len = read_file(argv[1], &source_text);

	// add expression support
	const character_t *input = "func f(x) {var y = 20 + 5; } func main(a, b, c) { var x = 40; }";

	token_t* tokens = lex(input);
	// ast_node_t* ast = parse(tokens);
	//
	// token_t example_tokens[] = {
	// 	{TOKEN_KEYWORD, {.str = "func"}},
	// 	{TOKEN_IDENTIFIER, {.str = "main"}},
	// 	{TOKEN_LPAREN, {.str = NULL}},
	// 	{TOKEN_RPAREN, {.str = NULL}},
	// 	{TOKEN_LBRACE, {.str = NULL}},
	// 	{TOKEN_KEYWORD, {.str = "var"}},
	// 	{TOKEN_IDENTIFIER, {.str = "x"}},
	// 	{TOKEN_ASSIGN, {.str = NULL}},
	// 	{TOKEN_NUMBER, {.number = 10}},
	// 	{TOKEN_SEMICOLON, {.str = NULL}},
	// 	// {TOKEN_KEYWORD, {.str = "if"}},
	// 	// {TOKEN_LPAREN, {.str = NULL}},
	// 	// {TOKEN_IDENTIFIER, {.str = "x"}},
	// 	// {TOKEN_RPAREN, {.str = NULL}},
	// 	// {TOKEN_LBRACE, {.str = NULL}},
	// 	// {TOKEN_IDENTIFIER, {.str = "x"}},
	// 	// {TOKEN_ASSIGN, {.str = NULL}},
	// 	// {TOKEN_NUMBER, {.number = 20}},
	// 	// {TOKEN_SEMICOLON, {.str = NULL}},
	// 	// {TOKEN_RBRACE, {.str = NULL}},
	// 	{TOKEN_RBRACE, {.str = NULL}},
	// 	{TOKEN_EOF, {.str = NULL}}
	// };
	//
	ASTNode* ast = parse_program(tokens);

	draw_ast(ast, "ast.png");

	printf("Parsed program successfully.\n");

	//
	// lexer_t* lexer = lex(source_text);
	// 
	// 

	// free(source_text);
	// free_lexer(lexer);
	free_tokens(tokens);
	free_ast(ast);
	return 0;
}
