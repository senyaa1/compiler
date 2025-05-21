#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "buffer.h"
#include "fs.h"
#include "io.h"
#include "graph.h"
#include "lexer.h"
#include "parser.h"
#include "preprocessor.h"
#include "translator.h"

/* TODO:
 * FIX memory leaks
 * FIX error handling (don't exit e.t.c.)
 */

const char* __asan_default_options() { return "detect_leaks=0"; }

int main(int argc, char **argv)
{
	setlocale(LC_ALL, "ru_RU.UTF-8");

	wchar_t *source_text = 0;
	size_t len = read_file(argv[1], &source_text);
	if(!len)
	{
		print_error("Could not open file\n");
		goto exit;
	}

	// if (preprocess(argv[1], &source_text))
	// 	goto exit;

	// printf("preprocessed text: %s\n", source_text);

	token_t *tokens = lex(source_text);

	ASTNode *ast = parse_program(tokens);

	// draw_ast(ast, "ast.png");

	printf("Parsed program successfully.\n");
	//
	buf_writer_t asm_buf = translate(ast);
	//
	printf("COMPILATION RESULT: \n\n%ls\n\n", asm_buf.buf);
	//
	// write_file("out.s", asm_buf.buf, strlen(asm_buf.buf));
	//
	// assemble("out.s", "a.out");
	// free(asm_buf.buf);

	free_tokens(tokens);
	free_ast(ast);
exit:
	free(source_text);
	return 0;
}
