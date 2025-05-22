#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "assembler.h"
#include "buffer.h"
#include "fs.h"
#include "graph.h"
#include "io.h"
#include "ir.h"
#include "lexer.h"
#include "parser.h"
#include "preprocessor.h"
#include "x86.h"

/* TODO:
 * FIX memory leaks
 * FIX error handling (don't exit e.t.c.)
 * FIX variable scope
 * refactor
 * change keywords
 * stdlib
 * factorial
 */

const char *__asan_default_options()
{
	return "detect_leaks=0";
}


int main(int argc, char **argv)
{
	setlocale(LC_ALL, "ru_RU.UTF-8");

	wchar_t *source_text = 0;
	// size_t len = read_file(argv[1], &source_text);
	// if (!len)
	// {
	// 	print_error("Could not open file\n");
	// 	free(source_text);
	// 	return 0;
	// }

	preprocess(argv[1], &source_text);

	token_t *tokens = lex(source_text);

	ast_node_t *ast = parse_program(tokens);
	draw_ast(ast, "out/ast.png");

	printf("Parsed program successfully.\n");

	wchar_t *ir_buf = translate_to_ir(ast);

	write_file("out/a.ir", ir_buf, wcslen(ir_buf));
	// printf("IR: \n\n%ls\n\n", ir_buf.buf);

	wchar_t *asm_buf = translate_ir_to_x86(ir_buf);

	// printf("x86 output: \n%ls\n", asm_buf);
	write_file("out/a.s", asm_buf, wcslen(asm_buf));

	assemble_nasm("out/a.s", "out/a.out");

	free(asm_buf);
	free(ir_buf);
	free_tokens(tokens);
	free_ast(ast);

	free(source_text);
	return 0;
}
