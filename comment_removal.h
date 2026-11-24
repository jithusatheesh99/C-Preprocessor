#ifndef COMMENT_REMOVAL_H
#define COMMENT_REMOVAL_H

// Removes comments from ONE line (the line is changed in place).
// in_comment remembers if we are inside a multi-line comment
// that started on an earlier line: 1 = yes, 0 = no.
void remove_comments(char *line, int *in_comment);

#endif
