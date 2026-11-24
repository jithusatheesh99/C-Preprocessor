#include <string.h>
#include "file_inclusion.h"
#include "comment_removal.h"
#include "macro_handler.h"
#include "common.h"

// Returns the line without its leading spaces
static char *skip_spaces(char *line)
{
    while (*line == ' ' || *line == '\t')
        line++;
    return line;
}

// Does the line start with this text (ignoring leading spaces)?
static int starts_with(char *line, const char *text)
{
    return strncmp(skip_spaces(line), text, strlen(text)) == 0;
}

// Handles a line like:  #include <myfile.h>   or   #include "myfile.h"
static void include_file(char *line, FILE *out)
{
    // find the first '<' or '"' and the matching closing character
    char *start = strpbrk(line, "<\"");
    char *end = NULL;
    if (start != NULL)
        end = strchr(start + 1, (*start == '<') ? '>' : '"');

    // bad #include line: just copy it
    if (start == NULL || end == NULL) {
        fprintf(out, "%s\n", line);
        return;
    }

    // copy the file name that is between the < > (or quotes)
    char name[200];
    int length = end - start - 1;
    if (length > 199) length = 199;
    strncpy(name, start + 1, length);
    name[length] = '\0';

    // Put the file's content here. We call process_file again (recursion),
    // so the included file also gets comments removed, macros, includes...
    if (process_file(name, out) == 0) {
        // file not found (for example <stdio.h>): keep the original line
        printf("Note: '%s' not found, keeping the #include line\n", name);
        fprintf(out, "%s\n", line);
    }
}

int process_file(const char *filename, FILE *out)
{
    FILE *in = fopen(filename, "r");
    if (in == NULL)
        return 0;

    char line[MAX_LINE];
    int in_comment = 0;      // are we inside a multi-line comment?

    // read the file ONE LINE at a time
    while (fgets(line, MAX_LINE, in) != NULL) {

        // remove the newline at the end (we add it back when writing)
        line[strcspn(line, "\r\n")] = '\0';

        // STEP 1: remove comments
        remove_comments(line, &in_comment);

        // STEP 2: decide what kind of line it is
        if (starts_with(line, "#include")) {
            include_file(line, out);             // paste the other file
        }
        else if (starts_with(line, "#define")) {
            add_macro(line);                     // remember it, write nothing
        }
        else if (*skip_spaces(line) == '#') {
            fprintf(out, "%s\n", line);          // other # lines: copy as they are
        }
        else {
            replace_macros(line);                // STEP 3: replace macros
            fprintf(out, "%s\n", line);          // write the final line
        }
    }

    fclose(in);
    return 1;
}
