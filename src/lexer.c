#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "lexer.h"

/*
    if
    else

    while

    var 
    func
    
    */

#define KEYWORDS        \
    KEYWORD(var)        \
    KEYWORD(func)       \
    KEYWORD(if)         \
    KEYWORD(else)       \
    KEYWORD(while)       \



static token_t create_token(token_type_t type)
{
    return (token_t){.type = type, .value = 0};
}

static token_t create_token_str(token_type_t type, const character_t* str) 
{
    return (token_t){.type = type, .value = (token_value_t){.str = strdup(str)}};
}

static token_t create_token_num(token_type_t type, number_t num) 
{
    return (token_t){.type = type, .value = (token_value_t){.number = num}};
}

static character_t current_char(lexer_t *lexer) 
{
    return lexer->pos < lexer->length ? lexer->input[lexer->pos] : '\0';
}

static void advance(lexer_t *lexer) 
{
    lexer->pos++;
}

static void skip_whitespace(lexer_t *lexer) 
{
    while (current_char(lexer) == ' ' || current_char(lexer) == '\n' || current_char(lexer) == '\t' || current_char(lexer) == '\r') 
        advance(lexer);
}

static bool is_keyword(character_t* identifier)
{
    #define KEYWORD(WORD)   strcmp(identifier, #WORD) == 0 ||

    return KEYWORDS 0;

    #undef KEYWORD
}


static character_t *read_identifier(lexer_t *lexer) 
{
    size_t start = lexer->pos;
    while ((current_char(lexer) >= 'a' && current_char(lexer) <= 'z') ||
        (current_char(lexer) >= 'A' && current_char(lexer) <= 'Z') ||
        (current_char(lexer) >= '0' && current_char(lexer) <= '9') ||
        current_char(lexer) == '_') {
        advance(lexer);
    }

    int length = lexer->pos - start;
    character_t *identifier = (character_t *)calloc(length + 1, sizeof(character_t));
    strncpy(identifier, lexer->input + start, length);
    identifier[length] = '\0';

    return identifier;
}

static number_t read_number(lexer_t *lexer) 
{
    static character_t num_buf[128] = { 0 };

    size_t start = lexer->pos;
    while (current_char(lexer) >= '0' && current_char(lexer) <= '9') 
        advance(lexer);

    size_t length = lexer->pos - start;
    strncpy(num_buf, lexer->input + start, length);
    num_buf[length] = '\0';

    character_t* end = 0;
    number_t number = strtod(num_buf, &end);
    return number;
}

#define TOKEN_CHAR(TOKEN, CHAR, STR)                 \
if (c == CHAR)                                      \
{                                                   \
    advance(lexer);                                 \
    return create_token_str(TOKEN, STR);            \
}                                                   \


token_t next_token(lexer_t *lexer) 
{
    skip_whitespace(lexer);
    character_t c = current_char(lexer);

    if (c == '\0') 
    {
        lexer->pos++; 
        return create_token(TOKEN_EOF);
    } 

    TOKEN_CHAR(TOKEN_LPAREN, '(', "(")
    TOKEN_CHAR(TOKEN_RPAREN, ')', ")")
    TOKEN_CHAR(TOKEN_LBRACE, '{', "{")
    TOKEN_CHAR(TOKEN_RBRACE, '}', "}")
    TOKEN_CHAR(TOKEN_COMMA, ',', ",")
    TOKEN_CHAR(TOKEN_SEMICOLON, ';', ";")
    TOKEN_CHAR(TOKEN_ASSIGN, '=', "=")
        
    TOKEN_CHAR(TOKEN_PLUS, '+', "+")
    TOKEN_CHAR(TOKEN_MINUS, '-', "-")
    TOKEN_CHAR(TOKEN_STAR, '*', "*")
    TOKEN_CHAR(TOKEN_SLASH, '/', "/")
    TOKEN_CHAR(TOKEN_CARET, '^', "^")

    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') 
    {
        character_t *identifier = read_identifier(lexer);
        if (is_keyword(identifier)) 
            return create_token_str(TOKEN_KEYWORD, identifier);

        return create_token_str(TOKEN_IDENTIFIER, identifier);
    } 

    if (c >= '0' && c <= '9') 
        return create_token_num(TOKEN_NUMBER, read_number(lexer));

    fprintf(stderr, "Unexpected character: %c\n", c);
    return create_token(TOKEN_EOF);
}

void print_token(token_t token)
{
    printf("Token (%d): ", token.type);
    
    if(token.type == TOKEN_NUMBER)
    {
        printf("%ld\n", token.value.number);
        return;
    }

    if(token.value.str) printf("%s\n", token.value.str);
}

void free_token(token_t token)
{
    if(token.value.str && token.type != TOKEN_NUMBER)
        free(token.value.str);
}

void free_tokens(token_t* tokens)
{
        size_t i = 0;
	do 
        {
		free_token(tokens[i]);
                i++;
	} while (tokens[i - 1].type != TOKEN_EOF);

        free(tokens);
}

token_t* lex(const char* text)
{
    lexer_t lexer = (lexer_t){text, 0, strlen(text)};

    size_t allocated = INITIAL_TOKEN_ALLOC, i = 0;
    token_t* tokens = (token_t*)calloc(allocated * sizeof(token_t), sizeof(token_t));

    do 
    {
        tokens[i] = next_token(&lexer);
        print_token(tokens[i]);
        i++;
        if(i >= allocated)
        {
            allocated *= 2;
            tokens = realloc(tokens, allocated * sizeof(token_t));
        }
    } while (tokens[i - 1].type != TOKEN_EOF);

    tokens = realloc(tokens, i * sizeof(token_t));

    return tokens;
}
