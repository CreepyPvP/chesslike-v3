#include "include/utils.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// the flen-th byte is 0
char* read_file(const char* file, i32* flen)
{
    char path_buffer[1024];
    strcpy(path_buffer, PATH_PREFIX);
    strcat(path_buffer, file);
    FILE* fptr = fopen(path_buffer, "rb");
    if (fptr == NULL) {
        printf("Failed to read file: %s\n", path_buffer);
        return NULL;
    }
    fseek(fptr, 0, SEEK_END);
    i32 len = ftell(fptr);
    char* buf = (char*) malloc(len + 1);
    fseek(fptr, 0, SEEK_SET);
    fread(buf, len, 1, fptr);
    buf[len] = 0;
    *flen = len;
    fclose(fptr);
    return buf;
}

