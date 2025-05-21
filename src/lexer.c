#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

#include "buffer.h"
#include "lexer.h"

static token_t create_token(token_type_t type)
{
	return (token_t){.type = type, .value = 0};
}

static token_t create_token_str(token_type_t type, const character_t *str)
{
	return (token_t){.type = type, .value = (token_value_t){.str = wcsdup(str)}};
}

static token_t create_token_num(token_type_t type, number_t num)
{
	return (token_t){.type = type, .value = (token_value_t){.number = num}};
}

static character_t current_char(lexer_t *lexer)
{
	return lexer->pos < lexer->length ? lexer->input[lexer->pos] : L'\0';
}

static void advance(lexer_t *lexer)
{
	lexer->pos++;
}

static void skip_whitespace(lexer_t *lexer)
{
	while (current_char(lexer) == L' ' || current_char(lexer) == L'\n' || current_char(lexer) == L'\t' ||
	       current_char(lexer) == L'\r')
		advance(lexer);
}

static bool is_keyword(character_t *identifier)
{
#define KEYWORD(WORD) wcscmp(identifier, WIDEN(#WORD)) == 0 ||
	return KEYWORDS 0;
#undef KEYWORD
}

static character_t *read_identifier(lexer_t *lexer)
{
	size_t start = lexer->pos;
	while ((iswalnum(current_char(lexer)) || current_char(lexer) == L'_'))
		advance(lexer);

	int length = lexer->pos - start;
	character_t *identifier = (character_t *)calloc(length + 1, sizeof(character_t));
	wcsncpy(identifier, lexer->input + start, length);
	identifier[length] = L'\0';

	return identifier;
}

static number_t read_number(lexer_t *lexer)
{
	static character_t num_buf[128] = {0};

	size_t start = lexer->pos;
	while (iswdigit(current_char(lexer)))
		advance(lexer);

	size_t length = lexer->pos - start;
	wcsncpy(num_buf, lexer->input + start, length);
	num_buf[length] = '\0';

	character_t *end = 0;
	number_t number = wcstod(num_buf, &end);
	return number;
}

#define TOKEN_CHAR(TOKEN, CHAR, STR)                                                                                   \
	if (c == CHAR)                                                                                                 \
	{                                                                                                              \
		advance(lexer);                                                                                        \
		return create_token_str(TOKEN, STR);                                                                   \
	}


token_t next_token(lexer_t *lexer)
{
	skip_whitespace(lexer);
	character_t c = current_char(lexer);

	if (c == L'\0')
	{
		lexer->pos++;
		return create_token(TOKEN_EOF);
	}

	TOKEN_CHAR(TOKEN_LPAREN, L'(', L"(")
	TOKEN_CHAR(TOKEN_RPAREN, L')', L")")
	TOKEN_CHAR(TOKEN_LBRACE, L'{', L"{")
	TOKEN_CHAR(TOKEN_RBRACE, L'}', L"}")
	TOKEN_CHAR(TOKEN_COMMA, L',', L",")
	TOKEN_CHAR(TOKEN_SEMICOLON, L';', L";")

	TOKEN_CHAR(TOKEN_PLUS, L'+', L"+")
	TOKEN_CHAR(TOKEN_MINUS, L'-', L"-")
	TOKEN_CHAR(TOKEN_STAR, L'*', L"*")
	TOKEN_CHAR(TOKEN_SLASH, L'/', L"/")

	TOKEN_CHAR(TOKEN_GREATER, L'>', L">")
	TOKEN_CHAR(TOKEN_LESS, L'<', L"<")

	TOKEN_CHAR(TOKEN_CARET, L'^', L"^")
	TOKEN_CHAR(TOKEN_AMPERSAND, L'&', L"&")
	TOKEN_CHAR(TOKEN_BAR, L'|', L"|")

	if (c == L'=')
	{
		advance(lexer);
		character_t next = current_char(lexer);
		if (next == L'=')
		{
			advance(lexer);
			return create_token_str(TOKEN_EQUALS, L"==");
		}

		return create_token_str(TOKEN_ASSIGN, L"=");
	}

	if (iswalpha(c) || c == L'_')
	{
		character_t *identifier = read_identifier(lexer);

		token_t tok = {};
		if (is_keyword(identifier))
			tok = create_token_str(TOKEN_KEYWORD, identifier);
		else
			tok = create_token_str(TOKEN_IDENTIFIER, identifier);

		free(identifier);
		return tok;
	}

	if (iswdigit(c))
		return create_token_num(TOKEN_NUMBER, read_number(lexer));


	if (c == L'\"')
	{
		advance(lexer);

		buf_writer_t writer = {.buf = calloc(64, sizeof(character_t)), .buf_len = 64, .cursor = 0};

		for (;;)
		{
			if (lexer->pos >= lexer->length)
			{
				fprintf(stderr, "error: unterminated string literal\n");
				exit(1);
			}


			character_t ch = current_char(lexer);

			if (ch == L'\"')
			{
				advance(lexer);
				break;
			}

			if (ch == L'\\')
			{
				advance(lexer);
				character_t ch = current_char(lexer);
				switch (ch)
				{
					case L'n':
						ch = L'\n';
						break;
					case L't':
						ch = L'\t';
						break;
					case L'r':
						ch = L'\r';
						break;
					case L'\\':
					case L'\"':
						break;
					default:
						fprintf(stderr, "unknown escape sequence\n");
						exit(1);
						break;
				}
			}

			advance(lexer);

			bufccpy(&writer, ch);
		}

		bufend(&writer);
		printf("lexed string \"%ls\"\n", writer.buf);
		return (token_t){.type = TOKEN_STRING, .value.str = writer.buf};
	}

	fprintf(stderr, "Unexpected character: %c\n", c);
	return create_token(TOKEN_EOF);
}

void print_token(token_t token)
{
	printf("Token (%d): ", token.type);

	if (token.type == TOKEN_NUMBER)
	{
		printf("%ld\n", token.value.number);
		return;
	}

	if (token.value.str)
		printf("%ls\n", token.value.str);
}

void free_token(token_t token)
{
	if (token.value.str && token.type != TOKEN_NUMBER)
		free(token.value.str);
}

void free_tokens(token_t *tokens)
{
	size_t i = 0;
	do
	{
		free_token(tokens[i]);
		i++;
	} while (tokens[i - 1].type != TOKEN_EOF);

	free(tokens);
}

token_t *lex(const character_t *text)
{
	lexer_t lexer = (lexer_t){text, 0, wcslen(text)};

	size_t allocated = INITIAL_TOKEN_ALLOC, i = 0;
	token_t *tokens = (token_t *)calloc(allocated * sizeof(token_t), sizeof(token_t));

	do
	{
		tokens[i] = next_token(&lexer);
		print_token(tokens[i]);
		i++;
		if (i >= allocated)
		{
			allocated *= 2;
			tokens = realloc(tokens, allocated * sizeof(token_t));
		}
	} while (tokens[i - 1].type != TOKEN_EOF);

	tokens = realloc(tokens, i * sizeof(token_t));

	return tokens;
}
