#include <string.h>
#include "file_inclusion.h"
#include "comment_removal.h"
#include "macro_handler.h"
#include "common.h"

static char *skip_spaces(char *line)
{
    while (*line == ' ' || *line == '\t')
        line++;
    return line;
}

static int starts_with(char *line, const char *text)
{
    return strncmp(skip_spaces(line), text, strlen(text)) == 0;
}

static void include_file(char *line, FILE *out)
{
    char *start = strpbrk(line, "<\"");
    char *end = NULL;
    if (start != NULL)
        end = strchr(start + 1, (*start == '<') ? '>' : '"');

    if (start == NULL || end == NULL) {
        fprintf(out, "%s\n", line);
        return;
    }

    char name[200];
    int length = end - start - 1;
    if (length > 199) length = 199;
    strncpy(name, start + 1, length);
    name[length] = '\0';

    if (process_file(name, out) == 0) {
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
    int in_comment = 0;      

    while (fgets(line, MAX_LINE, in) != NULL) {

        line[strcspn(line, "\r\n")] = '\0';

        remove_comments(line, &in_comment);

        if (starts_with(line, "#include")) {
            include_file(line, out);            
        }
        else if (starts_with(line, "#define")) {
            add_macro(line);                     
        }
        else if (*skip_spaces(line) == '#') {
            fprintf(out, "%s\n", line);          
        }
        else {
            replace_macros(line);                
            fprintf(out, "%s\n", line);        
        }
    }

    fclose(in);
    return 1;
}
