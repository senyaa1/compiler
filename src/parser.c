#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "debugger.h"

static token_t* tokens;
static size_t token_index;

static token_t* current_token() {
    return &tokens[token_index];
}

static void advance_token() {
    token_index++;
}

int expect_token(token_type_t type) {
    if (current_token()->type == type) {
        advance_token();
        return 1;
    }
    print_backtrace();
    fprintf(stderr, "Unexpected token: %d, while was expecting %d\n", current_token()->type, type);
    return 0;
}

// AST creation
ASTNode* create_ast_node(ast_node_type_t type) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    node->type = type;
    return node;
}

ASTNode* parse_block() {
    expect_token(TOKEN_LBRACE);
    ASTNode* block_node = create_ast_node(AST_BLOCK);
    block_node->data.block.children = NULL;
    block_node->data.block.child_count = 0;

    while (current_token()->type != TOKEN_RBRACE) {
        ASTNode* stmt = parse_statement();
        block_node->data.block.children = realloc(
            block_node->data.block.children,
            sizeof(ASTNode*) * (block_node->data.block.child_count + 1)
        );
        block_node->data.block.children[block_node->data.block.child_count++] = stmt;
    }

    expect_token(TOKEN_RBRACE);
    return block_node;
}

ASTNode* parse_statement() {
    if (current_token()->type == TOKEN_KEYWORD) {
        if (strcmp(current_token()->value.str, "var") == 0) {
            return parse_declaration();
        } else if (strcmp(current_token()->value.str, "func") == 0) {
            return parse_function();
        } else if (strcmp(current_token()->value.str, "if") == 0) {
            return parse_if();
        } else if (strcmp(current_token()->value.str, "while") == 0) {
            return parse_while();
        } else if (strcmp(current_token()->value.str, "out") == 0) {
            // return parse_();
        }else if (strcmp(current_token()->value.str, "in") == 0) {
            // return parse_while();
        }

    }

    return parse_expression();
}

ASTNode* parse_declaration() {
    expect_token(TOKEN_KEYWORD); // 'var'

    ASTNode* node = create_ast_node(AST_DECLARATION);
    node->data.declaration.identifier = create_ast_node(AST_IDENTIFIER);
    node->data.declaration.identifier->data.identifier = strdup(current_token()->value.str);
    expect_token(TOKEN_IDENTIFIER);

    if (current_token()->type == TOKEN_EQ) {
        advance_token();
        printf("parsing expression to init var\n");
        node->data.declaration.initializer = parse_expression();
    } else {
        printf("skipping initializer\n");
        node->data.declaration.initializer = NULL;
    }

    expect_token(TOKEN_SEMICOLON);
    return node;
}

ASTNode* parse_function() {
    expect_token(TOKEN_KEYWORD); // 'func'

    ASTNode* node = create_ast_node(AST_FUNCTION);
    node->data.function.name = create_ast_node(AST_IDENTIFIER);
    node->data.function.name->data.identifier = strdup(current_token()->value.str);
    expect_token(TOKEN_IDENTIFIER);

    expect_token(TOKEN_LPAREN);
    node->data.function.parameters = NULL;
    node->data.function.param_count = 0;

    while (current_token()->type != TOKEN_RPAREN) {
        ASTNode* param = create_ast_node(AST_IDENTIFIER);
        param->data.identifier = strdup(current_token()->value.str);
        expect_token(TOKEN_IDENTIFIER);

        node->data.function.parameters = realloc(
            node->data.function.parameters,
            sizeof(ASTNode*) * (node->data.function.param_count + 1)
        );
        node->data.function.parameters[node->data.function.param_count++] = param;

        if (current_token()->type == TOKEN_COMMA) {
            advance_token();
        } else {
            break;
        }
    }

    expect_token(TOKEN_RPAREN);
    node->data.function.body = parse_block();
    return node;
}

ASTNode* parse_if() {
    expect_token(TOKEN_KEYWORD); // 'if'

    ASTNode* node = create_ast_node(AST_IF);
    expect_token(TOKEN_LPAREN);
    node->data.if_statement.condition = parse_expression();
    expect_token(TOKEN_RPAREN);

    node->data.if_statement.then_branch = parse_block();

    if (current_token()->type == TOKEN_KEYWORD && strcmp(current_token()->value.str, "else") == 0) {
        advance_token();
        node->data.if_statement.else_branch = parse_block();
    } else {
        node->data.if_statement.else_branch = NULL;
    }

    return node;
}

ASTNode* parse_while() {
    expect_token(TOKEN_KEYWORD); // 'while'

    ASTNode* node = create_ast_node(AST_WHILE);
    expect_token(TOKEN_LPAREN);
    node->data.while_statement.condition = parse_expression();
    expect_token(TOKEN_RPAREN);

    node->data.while_statement.body = parse_block();
    return node;
}

ASTNode* parse_expression() {
    if (current_token()->type == TOKEN_IDENTIFIER && tokens[token_index + 1].type == TOKEN_EQ) {
        printf("parsing assignment\n");
        return parse_assignment();
    }
    return parse_math_expr();
    // return parse_primary();
}

ASTNode* parse_assignment() {
    ASTNode* node = create_ast_node(AST_ASSIGNMENT);

    // Parse left-hand side (identifier)
    node->data.assignment.left = create_ast_node(AST_IDENTIFIER);
    node->data.assignment.left->data.identifier = strdup(current_token()->value.str);
    expect_token(TOKEN_IDENTIFIER);

    // Consume '='
    expect_token(TOKEN_EQ);

    // Parse right-hand side (expression)
    node->data.assignment.right = parse_math_expr();

    return node;
}
ASTNode* parse_primary() 
{
    if (current_token()->type == TOKEN_NUMBER) {
        ASTNode* node = create_ast_node(AST_NUMBER);
        node->data.number = current_token()->value.number;
        advance_token();
        return node;
    } 
    if (current_token()->type == TOKEN_IDENTIFIER) {
        ASTNode* node = create_ast_node(AST_IDENTIFIER);
        node->data.identifier = strdup(current_token()->value.str);
        advance_token();
        return node;
    } 

    if (current_token()->type == TOKEN_LPAREN) {
        advance_token(); // Consume '('
        ASTNode* expr = parse_math_expr();
        if (current_token()->type != TOKEN_RPAREN) {
            fprintf(stderr, "Expected ')' after expression\n");
            return 0;
        }
        advance_token(); // Consume ')'
        return expr;
    } 
        fprintf(stderr, "Unexpected token: %d\n", current_token()->type);
            return 0;
    //
    // if (current_token()->type == TOKEN_NUMBER) {
    //     ASTNode* node = create_ast_node(AST_NUMBER);
    //     node->data.number = current_token()->value.number;
    //     expect_token(TOKEN_NUMBER);
    //     return node;
    // } 
    // 
    // if (current_token()->type == TOKEN_IDENTIFIER) {
    //     ASTNode* node = create_ast_node(AST_IDENTIFIER);
    //     node->data.identifier = strdup(current_token()->value.str);
    //     expect_token(TOKEN_IDENTIFIER);
    //     return node;
    // } 
    //
    // if (current_token()->type == TOKEN_PLUS) {
    //     ASTNode* node = create_ast_node(AST_BINARY);
    //     // node->data.identifier = strdup(current_token()->value.str);
    //     expect_token(TOKEN_PLUS);
    //     printf("PLUSSS\n");
    //     return node;
    // }
    //
    fprintf(stderr, "Unexpected token in primary: Type=%d, Value=%s\n", 
                current_token()->type, 
                current_token()->type == TOKEN_KEYWORD || current_token()->type == TOKEN_IDENTIFIER
                ? current_token()->value.str 
                : "N/A");
    return 0;
}
// Utility for freeing the AST
void free_ast(ASTNode* node) {
    if (!node) return;

    switch (node->type) {
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
            for (size_t i = 0; i < node->data.function.param_count; ++i) {
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
            for (size_t i = 0; i < node->data.block.child_count; ++i) {
                free_ast(node->data.block.children[i]);
            }
            free(node->data.block.children);
            break;
        case AST_PROGRAM:
            for (size_t i = 0; i < node->data.program.child_count; ++i) {
                free_ast(node->data.program.children[i]);
            }
            free(node->data.program.children);
            break;
        default:
            break;
    }

    free(node);
}


ASTNode* parse_program(token_t* t) 
{
	tokens = t;
	token_index = 0;

	ASTNode* program_node = create_ast_node(AST_PROGRAM);
	program_node->data.program.children = NULL;
	program_node->data.program.child_count = 0;

	while (current_token()->type != TOKEN_EOF) {
		ASTNode* stmt = parse_statement();
		program_node->data.program.children = realloc(
			program_node->data.program.children,
			sizeof(ASTNode*) * (program_node->data.program.child_count + 1)
		);
		program_node->data.program.children[program_node->data.program.child_count++] = stmt;
	}

	return program_node;
}

static ASTNode* create_binary_op_node(ASTNode* left, token_type_t op, ASTNode* right) {
    ASTNode* node = create_ast_node(AST_BINARY);
    node->data.binary_op.left = left;
    node->data.binary_op.op = op;
    node->data.binary_op.right = right;
    return node;
}


static ASTNode* parse_term() {
    ASTNode* node = parse_primary();
    if(!node) return 0;

    while (current_token()->type == TOKEN_STAR || current_token()->type == TOKEN_SLASH) {
        token_type_t op = current_token()->type;
        advance_token(); // Consume operator
        ASTNode* right = parse_primary();
        node = create_binary_op_node(node, op, right);
    }
    return node;
}

ASTNode* parse_math_expr() 
{
    ASTNode* node = parse_term();
    if(!node) return 0;

    while (current_token()->type == TOKEN_PLUS || current_token()->type == TOKEN_MINUS) {
        token_type_t op = current_token()->type;
        advance_token(); // Consume operator
        ASTNode* right = parse_term();
        node = create_binary_op_node(node, op, right);
    }
    return node;
}

