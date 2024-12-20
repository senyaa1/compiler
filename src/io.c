#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "io.h"

void print_line(const char* line, size_t line_len, size_t line_num)
{
	// fprintf(stderr, "   \t|  " RED BOLD "%*.s\n" RESET, , line);
	fprintf(stderr, "   %d\t|  " RED BOLD "%.*s\n" RESET, line_num, line_len, line);
	fprintf(stderr, "   \t|  " RED "^");

	for(int i = 1; i < line_len; i++)
		fputc('~', stderr);
	fprintf(stderr, RESET "\n");
}


void print_error(char* error)
{
	fprintf(stderr, RED BOLD "error: " RESET "%s\n" RESET, error);
}

void print_buf(uint8_t* buf, size_t sz)
{
	printf("\n");
	for(int i = 0; i < sz; i++)
		printf(YELLOW "%02hhX " RESET, buf[i]);
	printf("\n");
}
