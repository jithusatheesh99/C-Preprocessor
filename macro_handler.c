#include <string.h>
#include <ctype.h>
#include "macro_handler.h"
#include "common.h"

#define MAX_MACROS 100     // how many macros we can store
#define MAX_PARAMS 5       // how many arguments a macro can have
#define MAX_NAME   50      // longest macro / argument name

// Everything we remember about one macro
typedef struct {
    char name[MAX_NAME];                // e.g. "SQUARE"
    int  is_function;                   // 1 if it has arguments: SQUARE(x)
    int  num_params;                    // how many arguments
    char params[MAX_PARAMS][MAX_NAME];  // their names: "x"
    char value[MAX_LINE];               // what it becomes: "((x)*(x))"
} Macro;

static Macro macros[MAX_MACROS];        // the list of all macros
static int   macro_count = 0;           // how many are stored so far


/* ---------------- small helper functions ---------------- */

// Can this character be part of a word (name)?  letters, digits, underscore
static int is_word_char(char c)
{
    return isalnum((unsigned char)c) || c == '_';
}

// Can this character start a word?  (a word cannot start with a digit)
static int is_word_start(char c)
{
    return isalpha((unsigned char)c) || c == '_';
}

// Adds one character to the result
static void add_char(char *result, int *j, char c)
{
    if (*j < MAX_LINE - 1) {
        result[*j] = c;
        (*j)++;
    }
}

// Adds a whole text to the result
static void add_text(char *result, int *j, const char *text)
{
    for (int k = 0; text[k] != '\0'; k++)
        add_char(result, j, text[k]);
}

// Looks for a macro by name. Returns NULL if there is none.
// We search from the NEWEST to the oldest, so if a macro was
// defined twice, the latest definition wins.
static Macro *find_macro(const char *name)
{
    for (int k = macro_count - 1; k >= 0; k--) {
        if (strcmp(macros[k].name, name) == 0)
            return &macros[k];
    }
    return NULL;
}


/* ---------------- storing a macro ---------------- */

void add_macro(char *line)
{
    if (macro_count >= MAX_MACROS)
        return;

    Macro *m = &macros[macro_count];
    m->is_function = 0;
    m->num_params = 0;

    // Jump past the word "define"
    char *p = strstr(line, "define") + 6;
    while (*p == ' ' || *p == '\t') p++;

    // STEP 1: read the name
    int n = 0;
    while (is_word_char(*p) && n < MAX_NAME - 1)
        m->name[n++] = *p++;
    m->name[n] = '\0';

    // STEP 2: if a '(' comes right after the name, it has arguments
    if (*p == '(') {
        m->is_function = 1;
        p++;
        while (*p != ')' && *p != '\0') {
            if (is_word_char(*p) && m->num_params < MAX_PARAMS) {
                n = 0;
                while (is_word_char(*p) && n < MAX_NAME - 1)
                    m->params[m->num_params][n++] = *p++;
                m->params[m->num_params][n] = '\0';
                m->num_params++;
            } else {
                p++;               // skip commas and spaces
            }
        }
        if (*p == ')') p++;
    }

    // STEP 3: the rest of the line is the value
    while (*p == ' ' || *p == '\t') p++;
    strcpy(m->value, p);

    // remove spaces at the end of the value
    int len = strlen(m->value);
    while (len > 0 && isspace((unsigned char)m->value[len - 1]))
        m->value[--len] = '\0';

    macro_count++;
}


/* ---------------- using a macro ---------------- */

// For SQUARE(a+1) this reads the part inside the brackets.
// *pos points at the '(' and is moved to just after the ')'.
// The arguments are put in args[0], args[1], ... (split at commas).
// Returns how many arguments were found, or -1 if the ')' is missing.
static int read_arguments(const char *line, int *pos, char args[][MAX_LINE])
{
    int i = *pos + 1;      // start after the '('
    int depth = 1;         // counts brackets, so f((1,2),3) is split correctly
    int count = 0;         // which argument we are filling
    int n = 0;             // position inside that argument
    args[0][0] = '\0';

    while (line[i] != '\0') {
        char c = line[i];

        if (c == '(') depth++;
        if (c == ')') {
            depth--;
            if (depth == 0) break;        // this is the closing bracket
        }

        if (c == ',' && depth == 1) {     // a comma starts the next argument
            args[count][n] = '\0';
            count++;
            if (count >= MAX_PARAMS) return -1;
            n = 0;
            args[count][0] = '\0';
        } else if (c == ' ' && n == 0) {
            // ignore spaces at the start of an argument
        } else {
            args[count][n++] = c;
        }
        i++;
    }

    if (line[i] != ')') return -1;        // no closing bracket
    args[count][n] = '\0';
    *pos = i + 1;
    return count + 1;
}

// Writes the macro's value into the result, but every argument name
// (like x) is replaced by the real argument (like a+1).
static void write_value(Macro *m, char args[][MAX_LINE], char *result, int *j)
{
    int i = 0;
    while (m->value[i] != '\0') {
        if (is_word_start(m->value[i])) {
            // read one word of the value
            char word[MAX_NAME];
            int n = 0;
            while (is_word_char(m->value[i]) && n < MAX_NAME - 1)
                word[n++] = m->value[i++];
            word[n] = '\0';

            // is the word one of the argument names?
            int found = -1;
            for (int k = 0; k < m->num_params; k++)
                if (strcmp(word, m->params[k]) == 0)
                    found = k;

            if (found >= 0) add_text(result, j, args[found]);
            else            add_text(result, j, word);
        } else {
            add_char(result, j, m->value[i]);
            i++;
        }
    }
}

// Goes through the line once and replaces macros.
// Returns 1 if something was replaced, 0 if not.
static int replace_once(const char *line, char *result)
{
    int i = 0;             // position in the line
    int j = 0;             // position in the result
    int changed = 0;
    int in_string = 0;

    while (line[i] != '\0') {

        // inside "quotes": copy as it is, never replace
        if (in_string) {
            if (line[i] == '\\' && line[i + 1] != '\0')
                add_char(result, &j, line[i++]);
            if (line[i] == '"')
                in_string = 0;
            add_char(result, &j, line[i++]);
        }

        // opening quote
        else if (line[i] == '"') {
            in_string = 1;
            add_char(result, &j, line[i++]);
        }

        // a word: it might be a macro
        else if (is_word_start(line[i])) {
            char word[MAX_NAME];
            int n = 0;
            while (is_word_char(line[i]) && n < MAX_NAME - 1)
                word[n++] = line[i++];
            word[n] = '\0';

            Macro *m = find_macro(word);

            if (m == NULL) {
                // not a macro: keep the word
                add_text(result, &j, word);
            }
            else if (!m->is_function) {
                // simple macro, like PI: put its value
                add_text(result, &j, m->value);
                changed = 1;
            }
            else if (line[i] == '(') {
                // macro with arguments, like SQUARE(x)
                char args[MAX_PARAMS][MAX_LINE];
                int pos = i;
                int count = read_arguments(line, &pos, args);

                if (count == 1 && m->num_params == 0 && args[0][0] == '\0')
                    count = 0;                  // F() has no arguments

                if (count == m->num_params) {
                    write_value(m, args, result, &j);
                    i = pos;                    // continue after the ')'
                    changed = 1;
                } else {
                    add_text(result, &j, word); // wrong number of arguments
                }
            }
            else {
                add_text(result, &j, word);     // name without brackets
            }
        }

        // any other character
        else {
            add_char(result, &j, line[i++]);
        }
    }

    result[j] = '\0';
    return changed;
}

// A macro can use another macro (AREA uses SQUARE), so we repeat the
// replacing until nothing changes. The limit of 10 stops endless loops.
void replace_macros(char *line)
{
    char result[MAX_LINE];

    for (int round = 0; round < 10; round++) {
        int changed = replace_once(line, result);
        strcpy(line, result);
        if (!changed) break;
    }
}
