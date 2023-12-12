#define DEBUG

#include <stdint.h>

#ifdef WIN32
#define PATH_PREFIX "..\\..\\"
#elif
#define PATH_PREFIX "../"
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#define VK_CHECK(x)                                             \
do                                                              \
{                                                               \
    if (x != VK_SUCCESS)                                        \
    {                                                           \
        printf("Detected Vulkan error: %d\n", x);               \
        abort();                                                \
    }                                                           \
} while (0)

// typedef i8 Err;
//
// #define ASSERT(line, err, err_code) if (line) {     \
//     printf(err);                                    \
//     return err_code;                                \
// }
//
// #define ENSURE(line, err_code) if (line) {          \
//     return err_code;                                \
// }
