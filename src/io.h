#pragma once

#include <stdlib.h>
#include <stdint.h>

#define RED   "\x1B[31m"
#define GREEN   "\x1B[32m"
#define YELLOW   "\x1B[33m"
#define BLUE   "\x1B[34m"
#define MAGENTA   "\x1B[35m"
#define CYAN   "\x1B[36m"
#define WHITE   "\x1B[37m"
#define UNDERLINE "\e[4m"
#define RESET "\x1B[0m"
#define BOLD "\e[1m"


void print_line(const char* line, size_t line_len, size_t line_num);
void print_error(char* error);
void print_buf(uint8_t* buf, size_t sz);
