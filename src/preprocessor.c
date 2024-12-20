#include <stdio.h>
#include <string.h>

#include "preprocessor.h"
#include "io.h"
#include "buffer.h"
#include "lexer.h"


static int process_file(const char *file_path, int depth, buf_writer_t *writer);

static void strip_comments(char *line) 
{
	char *comment_start = strstr(line, "//");
	if (comment_start) 
		*comment_start = '\0';
}

static int process_line(char *line, int depth, buf_writer_t *writer) 
{
	strip_comments(line);

	if (strncmp(line, "#include", 8) == 0) 
	{
		char include_file[256];
		if (sscanf(line + 8, " \"%[^\"]\"", include_file) == 1) 
		{
			process_file(include_file, depth + 1, writer);
		} else {
			print_error("Malformed #include statement");
			return 1;
		}
		return 0;
	}

	bufncpy(writer, line);
	return 0;
}

int process_file(const char *file_path, int depth, buf_writer_t *writer) 
{
	if (depth > MAX_INCLUDE_DEPTH) 
	{
		print_error("Include depth exceeds limits\n");
		return 1;
	}

	FILE *file = fopen(file_path, "r");
	if (!file) 
	{
		print_error("Could not open file\n");
		return 1;
	}

	char line[MAX_LINE_LENGTH];
	while (fgets(line, MAX_LINE_LENGTH, file)) {
		char *newline = strchr(line, '\n');
		if (newline)	*newline = '\0';
		if(process_line(line, depth, writer))
			return 1;
	}

	fclose(file);

	return 0;
}

int preprocess(const char* file_path, char** buffer)
{
	buf_writer_t writer = { .buf = (character_t*)calloc(DEFAULT_PREPROCESSOR_ALLOC, sizeof(character_t)), .buf_len = DEFAULT_PREPROCESSOR_ALLOC, .cursor = 0, .indent = 0 };

	if(process_file(file_path, 0, &writer))
		return 1;

	bufend(&writer);
	*buffer = writer.buf;

	return 0;
}

