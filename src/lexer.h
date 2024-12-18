#pragma once

#include <stdlib.h>
#include <stdint.h>

static const size_t INITIAL_TOKEN_ALLOC = 128;

typedef int64_t number_t;
typedef char character_t;

typedef enum TOKEN_TYPES : uint8_t
{
    TOKEN_EOF = 0,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,
    TOKEN_ASSIGN,
    TOKEN_KEYWORD,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER
} token_type_t;

typedef union 
{
        number_t number;
        character_t* str;
} token_value_t;

typedef struct token 
{
    token_type_t type;
    token_value_t value;
} token_t;

typedef struct lexer 
{
    const character_t *input;
    size_t pos;
    size_t length;
} lexer_t;

token_t* lex(const char* text);
token_t next_token(lexer_t *lexer);
void print_token(token_t token);
void free_token(token_t token);
void free_tokens(token_t* tokens);
