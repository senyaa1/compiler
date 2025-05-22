#include <stdlib.h>
#include <stdio.h>

#define STDLIB_PATH "./std/libstd.o"

void assemble_nasm(const char* input_file, const char* output_file)
{
	char* cmd = 0;
	asprintf(&cmd, "nasm -f elf64 %s -o %s.o", input_file, input_file);
	system(cmd);
	free(cmd);

	asprintf(&cmd, "ld " STDLIB_PATH " %s.o -o %s", input_file, output_file);
	system(cmd);
	free(cmd);
}
