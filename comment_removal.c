#include <string.h>
#include "comment_removal.h"
#include "common.h"

void remove_comments(char *line, int *in_comment)
{
    char result[MAX_LINE];   
    int i = 0;              
    int j = 0;              
    int in_string = 0;       

    while (line[i] != '\0') {

        if (*in_comment) {
            if (line[i] == '*' && line[i + 1] == '/') {
                *in_comment = 0;      
                i = i + 2;
                result[j++] = ' ';
            } else {
                i++;                
            }
        }
        else if (in_string) {
            if (line[i] == '\\') {            
                result[j++] = line[i++];    
            } else if (line[i] == '"') {      
                in_string = 0;                
            }
            result[j++] = line[i++];
        }

        else if (line[i] == '"') {
            in_string = 1;
            result[j++] = line[i++];
        }

        else if (line[i] == '/' && line[i + 1] == '/') {
            break;
        }

        else if (line[i] == '/' && line[i + 1] == '*') {
            *in_comment = 1;
            i = i + 2;
        }

        else {
            result[j++] = line[i++];
        }
    }

    result[j] = '\0';
    strcpy(line, result);    
}
