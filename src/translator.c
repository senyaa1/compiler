#include "parser.h"
#include "translator.h"
#include "buffer.h"

void translate_recursive(buf_writer_t* writer, ASTNode* node)
{
	switch(node->type)
	{
	
	}
}


buf_writer_t translate(ASTNode* ast)
{
	buf_writer_t writer = { writer.buf = (char*)calloc(DEFAULT_ASM_ALLOC, sizeof(char)), .buf_len = DEFAULT_ASM_ALLOC};

	bufncpy(&writer, "; asm begin");

	translate_recursive(&writer, ast);

	bufncpy(&writer, "; end");

	return writer;
}
