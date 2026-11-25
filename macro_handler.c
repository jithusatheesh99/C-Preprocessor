#include <string.h>
#include <ctype.h>
#include "macro_handler.h"
#include "common.h"

#define MAX_MACROS 100     // how many macros we can store
#define MAX_PARAMS 5       // how many arguments a macro can have
#define MAX_NAME   50      // longest macro / argument name

typedef struct {
    char name[MAX_NAME];                
    int  is_function;                   
    int  num_params;                    
    char params[MAX_PARAMS][MAX_NAME];  
    char value[MAX_LINE];               
} Macro;

static Macro macros[MAX_MACROS];        
static int   macro_count = 0;           


static int is_word_char(char c)
{
    return isalnum((unsigned char)c) || c == '_';
}

static int is_word_start(char c)
{
    return isalpha((unsigned char)c) || c == '_';
}

static void add_char(char *result, int *j, char c)
{
    if (*j < MAX_LINE - 1) {
        result[*j] = c;
        (*j)++;
    }
}

static void add_text(char *result, int *j, const char *text)
{
    for (int k = 0; text[k] != '\0'; k++)
        add_char(result, j, text[k]);
}

static Macro *find_macro(const char *name)
{
    for (int k = macro_count - 1; k >= 0; k--) {
        if (strcmp(macros[k].name, name) == 0)
            return &macros[k];
    }
    return NULL;
}

void add_macro(char *line)
{
    if (macro_count >= MAX_MACROS)
        return;

    Macro *m = &macros[macro_count];
    m->is_function = 0;
    m->num_params = 0;

    char *p = strstr(line, "define") + 6;
    while (*p == ' ' || *p == '\t') p++;

    int n = 0;
    while (is_word_char(*p) && n < MAX_NAME - 1)
        m->name[n++] = *p++;
    m->name[n] = '\0';

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
                p++;
            }
        }
        if (*p == ')') p++;
    }

    while (*p == ' ' || *p == '\t') p++;
    strcpy(m->value, p);

    int len = strlen(m->value);
    while (len > 0 && isspace((unsigned char)m->value[len - 1]))
        m->value[--len] = '\0';

    macro_count++;
}


static int read_arguments(const char *line, int *pos, char args[][MAX_LINE])
{
    int i = *pos + 1;      
    int depth = 1;         
    int count = 0;         
    int n = 0;             
    args[0][0] = '\0';

    while (line[i] != '\0') {
        char c = line[i];

        if (c == '(') depth++;
        if (c == ')') {
            depth--;
            if (depth == 0) break;        
        }

        if (c == ',' && depth == 1) {    
            args[count][n] = '\0';
            count++;
            if (count >= MAX_PARAMS) return -1;
            n = 0;
            args[count][0] = '\0';
        } else if (c == ' ' && n == 0) {
        } else {
            args[count][n++] = c;
        }
        i++;
    }

    if (line[i] != ')') return -1;        
    args[count][n] = '\0';
    *pos = i + 1;
    return count + 1;
}

static void write_value(Macro *m, char args[][MAX_LINE], char *result, int *j)
{
    int i = 0;
    while (m->value[i] != '\0') {
        if (is_word_start(m->value[i])) {
            char word[MAX_NAME];
            int n = 0;
            while (is_word_char(m->value[i]) && n < MAX_NAME - 1)
                word[n++] = m->value[i++];
            word[n] = '\0';

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
static int replace_once(const char *line, char *result)
{
    int i = 0;             
    int j = 0;            
    int changed = 0;
    int in_string = 0;

    while (line[i] != '\0') {

        if (in_string) {
            if (line[i] == '\\' && line[i + 1] != '\0')
                add_char(result, &j, line[i++]);
            if (line[i] == '"')
                in_string = 0;
            add_char(result, &j, line[i++]);
        }

        else if (line[i] == '"') {
            in_string = 1;
            add_char(result, &j, line[i++]);
        }

        else if (is_word_start(line[i])) {
            char word[MAX_NAME];
            int n = 0;
            while (is_word_char(line[i]) && n < MAX_NAME - 1)
                word[n++] = line[i++];
            word[n] = '\0';

            Macro *m = find_macro(word);

            if (m == NULL) {
                add_text(result, &j, word);
            }
            else if (!m->is_function) {
                add_text(result, &j, m->value);
                changed = 1;
            }
            else if (line[i] == '(') {
                char args[MAX_PARAMS][MAX_LINE];
                int pos = i;
                int count = read_arguments(line, &pos, args);

                if (count == 1 && m->num_params == 0 && args[0][0] == '\0')
                    count = 0;                

                if (count == m->num_params) {
                    write_value(m, args, result, &j);
                    i = pos;                    
                    changed = 1;
                } else {
                    add_text(result, &j, word); 
                }
            }
            else {
                add_text(result, &j, word);     
            }
        }

        else {
            add_char(result, &j, line[i++]);
        }
    }

    result[j] = '\0';
    return changed;
}

void replace_macros(char *line)
{
    char result[MAX_LINE];

    for (int round = 0; round < 10; round++) {
        int changed = replace_once(line, result);
        strcpy(line, result);
        if (!changed) break;
    }
}
