#include <stdio.h>

#include "parser.h"
#include "translator.h"
#include "buffer.h"

typedef enum SCOPE 
{
	GLOBAL,
	LOCAL
} scope_t;

typedef struct context
{
	scope_t current_scope;
} context_t;

void translate_recursive(buf_writer_t* writer, ASTNode* node)
{
	if(!node) return;
	char* fmtbuf = 0;

	switch(node->type)
	{
		// case AST_IDENTIFIER:
		// 	bufcpy(writer, ";(ident)  ");
		// 	bufcpy(writer, node->data.identifier);
		// 	bufcpy(writer, "\n");
		// 	break;
		case AST_BLOCK:
			writer->indent++;
			for(int i = 0; i < node->data.block.child_count; i++)
				translate_recursive(writer, node->data.block.children[i]);
			writer->indent--;
			break;
		case AST_PROGRAM:
			for(int i = 0; i < node->data.program.child_count; i++)
				translate_recursive(writer, node->data.program.children[i]);
			break;
		case AST_DECLARATION:
			translate_recursive(writer, node->data.declaration.identifier);


			translate_recursive(writer, node->data.declaration.initializer);
			// bufncpy(writer, node->data.declaration.identifier.);
			break;
		case AST_NUMBER:
			asprintf(&fmtbuf, "push %ld", node->data.number);
			bufncpy(writer, fmtbuf);
			break;
		case AST_FUNCTION:
			bufncpy(writer, "; func");
			bufncpy(writer, "; args");
			for(int i = 0; i < node->data.function.param_count; i++)
			{
				bufncpy(writer, "; param");
				translate_recursive(writer, node->data.function.parameters[i]);
			}
			writer->indent++;
			translate_recursive(writer, node->data.function.body);
			writer->indent--;
			break;
		case AST_ASSIGNMENT:
			bufncpy(writer, "; var");
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
				default:
					break;
			}
			bufncpy(writer, "");
			break;
		case AST_IF:
			bufncpy(writer, "; begin if");
			writer->indent++;
			translate_recursive(writer, node->data.if_statement.condition);
			bufncpy(writer, "cmp");
			bufncpy(writer, "jz endif");
			translate_recursive(writer, node->data.if_statement.then_branch);
			writer->indent--;
			bufncpy(writer, "; end if");
			bufncpy(writer, "endif: ");
			break;
		case AST_WHILE:
			bufncpy(writer, "; begin while");
			bufncpy(writer, "whilebegin:");
			writer->indent++;

			translate_recursive(writer, node->data.while_statement.condition);
			bufncpy(writer, "cmp");
			bufncpy(writer, "jnz whileend");

			translate_recursive(writer, node->data.while_statement.body);

			writer->indent--;
			bufncpy(writer, "whileeend:");
			bufncpy(writer, "; end while");
			break;
		default:
			fprintf(stderr, "unknown node! (%d)\n", node->type);
			break;
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
