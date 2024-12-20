#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INCLUDE_DEPTH 50
#define MAX_LINE_LENGTH 1024
#define MAX_DEFINES 100

void process_file(const char *file_path, int depth);

void process_line(char *line, FILE *output, int depth) 
{
    char buffer[MAX_LINE_LENGTH];

    if (strncmp(line, "#include", 8) == 0) {
        char include_file[256];
        if (sscanf(line + 8, " \"%[^\"]\"", include_file) == 1) {
            process_file(include_file, depth + 1);
        } else {
            fprintf(stderr, "Malformed #include statement: %s\n", line);
            exit(1);
        }
        return;
    }

    fprintf(output, "%s\n", line);
}


void process_file(const char *file_path, int depth) 
{
    if (depth > MAX_INCLUDE_DEPTH) {
        fprintf(stderr, "Include depth exceeds %d levels\n", MAX_INCLUDE_DEPTH);
        exit(1);
    }

    FILE *file = fopen(file_path, "r");
    if (!file) {
        fprintf(stderr, "Could not open file: %s\n", file_path);
        exit(1);
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, MAX_LINE_LENGTH, file)) {
        char *newline = strchr(line, '\n');
        if (newline) {
            *newline = '\0';
        }
        process_line(line, stdout, depth);
    }

    fclose(file);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <source file>\n", argv[0]);
        return 1;
    }

    process_file(argv[1], 0);
    return 0;
}
