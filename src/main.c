#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "graph.h"
#include "buffer.h"
#include "translator.h"
#include "preprocessor.h"
#include "fs.h"

/*	TODO:
 *	frontend
 *		build ast
 *		save as json
 *	middleend	// last 
 *		optimize math expressions
 *		forcefully differentiate all expressions
 *	backend
 *		translator
 */

/*	syntax example
	
 
	*/


void assemble(const char* asm_path, const char* out_path)
{
	char* command = 0;
	asprintf(&command, "./asm %s -o %s", asm_path, out_path);
	system(command);
	free(command);
}

const char* __asan_default_options() { return "detect_leaks=0"; }


int main(int argc, char** argv)
{
	char* source_text = 0;
	// size_t len = read_file(argv[1], &source_text);

	if(preprocess(argv[1], &source_text))
		goto exit;

	printf("preprocessed text: %s\n", source_text);

	token_t* tokens = lex(source_text);

	ASTNode* ast = parse_program(tokens);

	draw_ast(ast, "ast.png");

	printf("Parsed program successfully.\n");

	buf_writer_t asm_buf = translate(ast);

	printf("COMPILATION RESULT: \n\n%s\n\n", asm_buf.buf);

	write_file("out.s", asm_buf.buf, strlen(asm_buf.buf));

	assemble("out.s", "a.out");

	free_tokens(tokens);
	free_ast(ast);
	free(asm_buf.buf);
exit:
	free(source_text);
	return 0;
}
