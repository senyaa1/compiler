#include <limits.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

#include "buffer.h"
#include "preprocessor.h"

static buf_writer_t preprocessor_buf = {};

static FILE *wfopen_utf8(const wchar_t *path, const wchar_t *mode)
{
	size_t needed = wcstombs(NULL, path, 0) + 1;
	char *mb = calloc(needed, sizeof(char));
	wcstombs(mb, path, needed);

	FILE *fp = fopen(mb, (const char *)mode);
	free(mb);

	return fp;
}

static wchar_t *mbs_to_wcs(const char *src)
{
	if (!src)
		return NULL;
	size_t need = mbstowcs(NULL, src, 0);
	if (need == (size_t)-1)
		return NULL;
	wchar_t *dst = calloc((need + 1), sizeof(wchar_t));
	mbstowcs(dst, src, need + 1);
	return dst;
}

static void preprocess_file(const wchar_t *fname, int depth)
{
	if (depth > MAX_DEPTH)
	{
		fwprintf(stderr, L"include depth exceeded: %ls\n", fname);
		exit(EXIT_FAILURE);
	}

	FILE *fp = wfopen_utf8(fname, L"r, ccs=UTF-8");
	if (!fp)
	{
		fwprintf(stderr, L"cannot open %ls\n", fname);
		exit(EXIT_FAILURE);
	}

	wchar_t dir[PATH_MAX] = L"";
	const wchar_t *sep = wcsrchr(fname, PATH_SEP);
	if (sep)
	{
		wcsncpy(dir, fname, sep - fname);
		dir[sep - fname] = 0;
	}

	bool in_block = false;
	wchar_t line[MAX_LINE];
	while (fgetws(line, MAX_LINE, fp))
	{
		wchar_t cleaned[MAX_LINE];
		size_t o = 0;
		for (size_t i = 0; line[i] && o + 1 < MAX_LINE; ++i)
		{
			if (in_block)
			{
				if (line[i] == L'*' && line[i + 1] == L'/')
				{
					in_block = false;
					++i;
				}
				continue;
			}
			if (line[i] == L'/' && line[i + 1] == L'*')
			{
				in_block = true;
				++i;
				continue;
			}
			if (line[i] == L'/' && line[i + 1] == L'/')
			{
				break;
			}
			cleaned[o++] = line[i];
		}
		cleaned[o] = 0;

		wchar_t *p = cleaned;
		while (*p && iswspace(*p))
			++p;

		if (*p == L'#')
		{
			++p;
			while (iswspace(*p))
				++p;
			if (wcsncmp(p, L"include", 7) == 0)
			{
				p += 7;
				while (iswspace(*p))
					++p;
				if (*p == L'"')
				{
					++p;
					wchar_t inc[PATH_MAX];
					size_t j = 0;
					while (*p && *p != L'"' && j + 1 < PATH_MAX)
						inc[j++] = *p++;
					inc[j] = 0;
					if (*p == L'"')
					{
						wchar_t full[PATH_MAX];
						if (dir[0])
							swprintf(full, PATH_MAX, L"%ls%c%ls", dir, PATH_SEP, inc);
						else
							wcsncpy(full, inc, PATH_MAX);
						preprocess_file(full, depth + 1);
						continue;
					}
				}
			}
		}

		bufcpy(&preprocessor_buf, cleaned);
		// fputws(cleaned, stdout);
	}

	fclose(fp);
}


static void append_elf64_start_function(buf_writer_t *writer)
{
	bufncpy(writer, L"		\
	func _start()			\
	{				\
		main();			\
		asm \"mov edi, ecx\";	\
		asm \"mov eax, 60\";	\
		asm \"syscall\";	\
	}");
}


int preprocess(const char *file_path, wchar_t **buffer)
{
	if (preprocessor_buf.buf)
		free(preprocessor_buf.buf);

	preprocessor_buf = (buf_writer_t){.buf = calloc(1024, sizeof(wchar_t)), .buf_len = 1024};

	preprocess_file((const wchar_t *)mbs_to_wcs(file_path), 0);

	append_elf64_start_function(&preprocessor_buf);

	*buffer = preprocessor_buf.buf;
}
