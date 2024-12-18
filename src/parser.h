#pragma once

#include "lexer.h"

typedef enum AST_NODE_TYPE {
    AST_NUMBER,
    AST_IDENTIFIER,
    AST_ASSIGNMENT,
    AST_DECLARATION,
    AST_FUNCTION,
    AST_IF,
    AST_WHILE,
    AST_BLOCK,
    AST_PROGRAM
} ast_node_type_t;

typedef struct ASTNode {
    ast_node_type_t type;
    union {
        number_t number;
        character_t* identifier;
        struct {
            struct ASTNode* left;
            struct ASTNode* right;
        } assignment;
        struct {
            struct ASTNode* identifier;
            struct ASTNode* initializer;
        } declaration;
        struct {
            struct ASTNode* condition;
            struct ASTNode* then_branch;
            struct ASTNode* else_branch;
        } if_statement;
        struct {
            struct ASTNode* condition;
            struct ASTNode* body;
        } while_statement;
        struct {
            struct ASTNode* name;
            struct ASTNode** parameters;
            size_t param_count;
            struct ASTNode* body;
        } function;
        struct {
            struct ASTNode** children;
            size_t child_count;
        } block;
        struct {
            struct ASTNode** children;
            size_t child_count;
        } program;
    } data;
} ASTNode;


ASTNode* parse_block();
ASTNode* parse_statement();
ASTNode* parse_expression();
ASTNode* parse_assignment();
ASTNode* parse_declaration();
ASTNode* parse_function();
ASTNode* parse_if();
ASTNode* parse_while();
ASTNode* parse_primary();
ASTNode* parse_program(token_t* tokens);
void free_ast(ASTNode* node);
