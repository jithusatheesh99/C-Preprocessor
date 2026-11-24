#include <stdio.h>
#include <string.h>
#include "file_inclusion.h"

int main(int argc, char *argv[])
{
    // we need exactly one argument: the input file
    if (argc != 2) {
        printf("Usage: %s inputfile.c\n", argv[0]);
        return 1;
    }

    // make the output name: inputfile.c  ->  inputfile.i
    char outname[300];
    strncpy(outname, argv[1], 290);
    outname[290] = '\0';

    char *dot = strrchr(outname, '.');      // last '.' in the name
    if (dot != NULL)
        *dot = '\0';                        // cut the old extension
    strcat(outname, ".i");

    // open the output file and do all the work
    FILE *out = fopen(outname, "w");
    if (out == NULL) {
        printf("Cannot create %s\n", outname);
        return 1;
    }

    if (process_file(argv[1], out) == 0) {
        printf("Cannot open input file %s\n", argv[1]);
        fclose(out);
        remove(outname);
        return 1;
    }

    fclose(out);
    printf("Created %s\n", outname);
    return 0;
}
