#include <stdio.h>
#include <string.h>
#include "grail.h"

#define ENTRY_BUFSIZE 4096
#define FILENAME_SIZE 1024
#define _DEBUG

const char *grail_cmd = "grail";
static char bufsize[ENTRY_BUFSIZE];
static int entry;

int main(int argc, char *argv[]) 
{
    size_t entry_len = strlen(grail_cmd);

    if (argc <= 2) {
        printf("Expected at least one argument\n");

        return 0;
    }

    if (argc > 3) {
        printf("Too many arguments supplied: %d\n", argc);

        return 0;
    }

    entry = memcmp(argv[1], grail_cmd, entry_len);

    if (entry == 0) {
       char *file;

       process_templates(argv[2]); 

       fprintf(stdin, "Success\n");

       return 0;
    }  

    fprintf(stderr, "Error");

    return 0;
}