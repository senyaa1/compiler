#include "parser.h"
#include "lexer.h"

#define NUM(NUM)				node_create_num_d(NUM)

#define TRIV_NUM(NODE, VAL, ACTION)					\
if(NODE && NODE->type == NODE_NUMBER && NODE->value.number == VAL)	\
{									\
	(*optimization_cnt)++;						\
	ACTION;								\
}									\


#define PERFORM_OP(TYPE, OPERATION)					\
case TYPE:								\
	(*optimization_cnt)++;						\
	return NUM(left_num OPERATION right_num);			\


#define ISNUM(NODE)	(NODE && NODE->type == TOKEN_NUMBER)
#define ISFUNC(NODE)	(NODE && NODE->type == TOKEN_NUMBER)

#define TRY_GET_NUM(NODE, CHILD1)				\
if(!second_const && NODE->CHILD1 && NODE->CHILD1->type == NODE_NUMBER)	\
{									\
	second_const = NODE->CHILD1->value.number;			\
	func = NODE;							\
	to_modify = &NODE->CHILD1->value.number;			\
}									\

#define OPT_OP(OPERATION_NAME, OPERATION)						\
case OPERATION_NAME:									\
	(*optimization_cnt)++;								\
	*to_modify = (first_const OPERATION second_const);				\
	return func;									\

//
// static ASTNode* optimize_recursive(ASTNode* node, size_t* optimization_cnt)
// {
// 	if(!node) return 0;
//
// 	if(node->type != AST_BINARY)
// 		return node;
//
// 	ASTNode* LEFT = node->data.binary_op.left, *RIGHT = node->data.binary_op.right;
//
// 	// Trivial (*0, +0, etc)
// 	switch(node->data.binary_op.op)
// 	{
// 		case TOKEN_PLUS:
// 		case TOKEN_MINUS:
// 			TRIV_NUM(RIGHT, 0,	return LEFT);
// 			TRIV_NUM(LEFT, 0,	return RIGHT);
// 			break;
//
// 		case TOKEN_STAR:
// 			TRIV_NUM(RIGHT, 1,	return LEFT);
// 			TRIV_NUM(LEFT, 1,	return RIGHT);
//
// 			TRIV_NUM(RIGHT, 0,	return NUM(0));
// 			TRIV_NUM(LEFT, 0,	return NUM(0));
// 			break;
// 		case TOKEN_SLASH:
// 			TRIV_NUM(RIGHT, 1,	return LEFT);
// 			break;
// 		default:
// 			break;
// 	}
//
// 	// Calculate constants (e.g. 2 + 2)
// 	if(ISNUM(LEFT) && ISNUM(RIGHT))
// 	{					
// 		double left_num = LEFT->data.number, right_num = RIGHT->data.number;
// 		switch(node->data.binary_op.op)
// 		{
// 			PERFORM_OP(TOKEN_PLUS, +)
// 			PERFORM_OP(TOKEN_MINUS, -)
// 			PERFORM_OP(TOKEN_STAR, *)
// 			PERFORM_OP(TOKEN_SLASH, /)
// 			case POW:
// 				(*optimization_cnt)++;
// 				node_free(node);
// 				return NUM(pow(left_num, right_num));
// 			default:
// 				break;
// 		}
// 	}
//
// 	node->right = optimize_recursive(node->right, optimization_cnt);
// 	node->left = optimize_recursive(node->left, optimization_cnt);
//
// 	return node;
// }
//
// ASTNode* optimize(diff_node_t* tree, buf_writer_t* writer)
// {
// 	bufcpy(writer, "Давайте немного упростим данное выражение.\n");
// 	size_t optimization_cnt = 0;
// 	do 
// 	{
// 		optimization_cnt = 0;
// 		tree = optimize_recursive(tree, &optimization_cnt);
// 	} while (optimization_cnt > 0);
//   
// 	return tree;
// }


