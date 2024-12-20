#include <graphviz/cgraph.h>
#include <stdlib.h>

#include <graphviz/gvc.h>

#include "graph.h"
#include "parser.h"

static Agnode_t* create_node(Agraph_t* g)
{
	Agnode_t* node = agnode(g, 0, 1);
	agsafeset(node, "color", "white", "");
	agsafeset(node, "style", "rounded", "");
	agsafeset(node, "shape", "rect", "");
	agsafeset(node, "fontcolor", "white", "");

	return node;
}

static Agnode_t* render_node(Agraph_t* g, ASTNode* node)
{
	if(!node) return 0;
	Agnode_t* root = create_node(g);

	char* label = 0;
	switch(node->type)
	{
		case AST_NUMBER:
			asprintf(&label, "%ld", node->data.number);
			agsafeset(root, "color", "green", "");
			break;
		case AST_ASSIGNMENT:
			asprintf(&label, "=");
			agsafeset(root, "color", "red", "");
			agedge(g, root, render_node(g, node->data.assignment.left), 0, 1);
			agedge(g, root, render_node(g, node->data.assignment.right), 0, 1);
			break;
		case AST_DECLARATION:
			agsafeset(root, "color", "cyan", "");
			asprintf(&label, "vardecl");
			agedge(g, root, render_node(g, node->data.declaration.identifier), 0, 1);
			agedge(g, root, render_node(g, node->data.declaration.initializer), 0, 1);
			break;
		case AST_FUNCTION:
			agsafeset(root, "color", "cyan", "");
			asprintf(&label, "function");
			agset(agedge(g, root, render_node(g, node->data.function.name), 0, 1), "color", "green");
			for (size_t i = 0; i < node->data.function.param_count; ++i) 
			{
				Agedge_t* edge = agedge(g, root, render_node(g, node->data.function.parameters[i]), 0, 1);
				agsafeset(edge, "color", "cyan", "");
				agsafeset(edge, "label", "param", "");
			}
			agedge(g, root, render_node(g, node->data.function.body), 0, 1);
			break;
		case AST_FUNCTION_CALL:
			agsafeset(root, "color", "cyan", "");
			asprintf(&label, "call");
			agset(agedge(g, root, render_node(g, node->data.function_call.name), 0, 1), "color", "green");
			for (size_t i = 0; i < node->data.function_call.arg_count; ++i) 
			{
				Agedge_t* edge = agedge(g, root, render_node(g, node->data.function_call.arguments[i]), 0, 1);
				agsafeset(edge, "color", "cyan", "");
				agsafeset(edge, "label", "param", "");
			}
			break;
		case AST_IF:
			agsafeset(root, "color", "red", "");
			asprintf(&label, "if");
			agedge(g, root, render_node(g, node->data.if_statement.condition), 0, 1);
			agedge(g, root, render_node(g, node->data.if_statement.then_branch), 0, 1);
			// agedge(g, root, render_node(g, node->data.if_statement.else_branch), 0, 1);
			break;
		case AST_WHILE:
			agsafeset(root, "color", "red", "");
			asprintf(&label, "while");
			agedge(g, root, render_node(g, node->data.while_statement.condition), 0, 1);
			agedge(g, root, render_node(g, node->data.while_statement.body), 0, 1);
			break;
		case AST_BLOCK:
			asprintf(&label, "block");
			for (size_t i = 0; i < node->data.block.child_count; ++i) 
				agedge(g, root, render_node(g, node->data.block.children[i]), 0, 1);
			break;
		case AST_PROGRAM:
			asprintf(&label, "program");
			for (size_t i = 0; i < node->data.program.child_count; ++i) 
				agedge(g, root, render_node(g, node->data.program.children[i]), 0, 1);
			break;
		case AST_IDENTIFIER:
			asprintf(&label, "ident: %s", node->data.identifier);
			break;
		case AST_BINARY:
			asprintf(&label, "op: %d", node->data.binary_op.op);
			agsafeset(root, "color", "red", "");
			agedge(g, root, render_node(g, node->data.binary_op.left), 0, 1);
			agedge(g, root, render_node(g, node->data.binary_op.right), 0, 1);
			break;
		case AST_INLINE_ASM:
			asprintf(&label, "inline asm: %s", node->data.identifier);
			agsafeset(root, "color", "purple", "");
			agsafeset(root, "label", label, "");
			break;
		case AST_RETURN:
			asprintf(&label, "return");
			agsafeset(root, "color", "cyan", "");
			if(node->data.return_statement.value)
				agedge(g, root, render_node(g, node->data.return_statement.value), 0, 1);
			break;

		default:
			asprintf(&label, "unknown (%d)", node->type);
			agsafeset(root, "color", "blue", "");
			break;
	}

	agset(root, "label", label);

	// if(node)
	// {
	// 	Agnode_t* left_node = render_node(g, node->left);
	// 	Agedge_t* l_edge = agedge(g, root, left_node, 0, 1);
	// 	agsafeset(l_edge, "color", "white", "");
	// }
	//
	// if(node->right)
	// {
	// 	Agnode_t* right_node = render_node(g, node->right);
	// 	Agedge_t* r_edge = agedge(g, root, right_node, 0, 1);
	// 	agsafeset(r_edge, "color", "white", "");
	// }

	return root;
}


void draw_ast(ASTNode* ast, const char* output_filename)
{
	GVC_t *gvc = gvContext();

	Agraph_t *g = agopen("G", Agdirected, 0);

	agsafeset(g, "bgcolor", "gray12", "");

	render_node(g, ast);
	// render_obj(g, json->value.obj);

	gvLayout(gvc, g, "dot");

	FILE *file = fopen(output_filename, "wb");
	gvRender(gvc, g, "png", file);

	agclose(g);
	gvFreeContext(gvc);
}
