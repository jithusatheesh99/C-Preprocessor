#ifndef MACRO_HANDLER_H
#define MACRO_HANDLER_H

// Reads a line like "#define PI 3.14" or "#define SQUARE(x) ((x)*(x))"
// and remembers the macro.
void add_macro(char *line);

// Replaces every macro found in the line (the line is changed in place).
void replace_macros(char *line);

#endif
