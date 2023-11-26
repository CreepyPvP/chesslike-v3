#ifndef DEFINES_H
#define DEFINES_H

#include <stdint.h>
#include <assert.h>

#define DEBUG

#ifdef DEBUG
#define GL(line) do {                      \
        line;                                 \
        assert(glGetError() == GL_NO_ERROR);  \
    } while(0)
#else
#  define GL(line) line
#endif

#define TRUE 1
#define FALSE 0

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#endif
