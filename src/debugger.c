#define __USE_GNU

#include <signal.h>
#include <ucontext.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <execinfo.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "debugger.h"

#define BACKTRACE_MAX_SZ 100

void print_backtrace()
{
	void *buffer[BACKTRACE_MAX_SZ];
	int nptrs = backtrace(buffer, BACKTRACE_MAX_SZ);
	char** strings = backtrace_symbols(buffer, nptrs);

	if (strings == NULL) {
		perror("backtrace_symbols");
		exit(EXIT_FAILURE);
	}

	fprintf(stderr, "Backtrace (%d):\n", nptrs);
	fprintf(stderr, "-------------------------------------------------------\n");
	for (int j = 0; j < nptrs; j++)
		fprintf(stderr, "|-%s\n", strings[j]);
	fprintf(stderr, "\n\n");
        free(strings); 

}
