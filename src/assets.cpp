#include "include/assets.h"
#include "include/defines.h"
#include "include/utils.h"

#include <stdio.h>
#include <stdlib.h>


// returns true if ptr starts with prefix. 
// Also removes the prefix string from ptr
bool prefix(const char* prefix, const char** ptr)
{
    i32 i = 0;
    while (prefix[i]) {
        if (prefix[i] != (*ptr)[i])
            return false;
        ++i;
    }
    (*ptr) += i;
    return true;
}

void skip_whitespaces(const char** ptr)
{
    while (**ptr == ' ') {
        (*ptr)++;
    }
}

void next_line(const char** ptr) 
{
    while (**ptr != '\n' && **ptr != 0) {
        (*ptr)++;
    }
    // praise windwos
    while (**ptr == '\n' || **ptr == '\r') {
        (*ptr)++;
    }
}

i32 read_version(const char** ptr)
{
    i32 version = atoi(*ptr);
    next_line(ptr);
    return version;
}

void read_model(const char** ptr)
{
    skip_whitespaces(ptr);
}

void load_scene(const char* file) 
{
    i32 len;
    const char* content = read_file(file, &len);
    if (!content) {
        printf("Failed to load scene: %s\n", file);
        exit(1);
    }
    printf("Parsing scene: %s\n", file);
    i32 version = read_version(&content);
    const char** ptr = &content;
    while (0) {
        if (prefix("MODEL", ptr)) {

        }
        if (prefix("OBJECT", ptr)) {

        }
    }
}
