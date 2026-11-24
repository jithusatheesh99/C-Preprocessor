#ifndef FILE_INCLUSION_H
#define FILE_INCLUSION_H

#include <stdio.h>

// Processes a whole file and writes the result to 'out'.
// Returns 1 if OK, or 0 if the file could not be opened.
int process_file(const char *filename, FILE *out);

#endif
