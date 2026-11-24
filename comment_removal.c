#include <string.h>
#include "comment_removal.h"
#include "common.h"

void remove_comments(char *line, int *in_comment)
{
    char result[MAX_LINE];   // the cleaned line is built here
    int i = 0;               // position in the original line
    int j = 0;               // position in the result
    int in_string = 0;       // 1 while we are inside "quotes"

    while (line[i] != '\0') {

        // CASE 1: inside a multi-line comment -> throw characters away
        //         until we see the closing star-slash
        if (*in_comment) {
            if (line[i] == '*' && line[i + 1] == '/') {
                *in_comment = 0;      // the comment is over
                i = i + 2;
                result[j++] = ' ';
            } else {
                i++;                  // skip this character
            }
        }

        // CASE 2: inside "quotes" -> copy everything, a // here is just text
        else if (in_string) {
            if (line[i] == '\\') {            // an escape such as \"
                result[j++] = line[i++];      // copy the backslash, the next
            } else if (line[i] == '"') {      // character is copied below
                in_string = 0;                // the closing quote
            }
            result[j++] = line[i++];
        }

        // CASE 3: an opening quote
        else if (line[i] == '"') {
            in_string = 1;
            result[j++] = line[i++];
        }

        // CASE 4: // starts a single-line comment -> ignore the rest of the line
        else if (line[i] == '/' && line[i + 1] == '/') {
            break;
        }

        // CASE 5: slash-star starts a multi-line comment
        else if (line[i] == '/' && line[i + 1] == '*') {
            *in_comment = 1;
            i = i + 2;
        }

        // CASE 6: a normal character -> keep it
        else {
            result[j++] = line[i++];
        }
    }

    result[j] = '\0';
    strcpy(line, result);    // put the cleaned line back
}
