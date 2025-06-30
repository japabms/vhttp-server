#ifndef _V_UTIL_H_
#define _V_UTIL_H_

#define VStringify_(S) #S
#define VStringify(S) Stringify_(S)

#define VConcat_(S1, S2) S1##S2
#define VConcat(S1, S2) VConcat_(S1, S2)

#define VLocalError() fprintf(stderr, " on file %s.c, function %s, line %d\n", __FILE__, __PRETTY_FUNCTION__, __LINE__)

#define VError(s)                               \
  do {                                          \
    if (errno != 0) {                           \
      perror("[ERRNO]");                        \
    }                                           \
    fprintf(stderr, "[ERROR] %s", (s));         \
    VLocalError();                              \
    exit(EXIT_FAILURE);                         \
  } while (0);                                  \

#define VTodo(s) fprintf(stdout, "%s:%d TODO: %s\n", __FILE__, __LINE__, s)


#include <stdint.h>
#include <limits.h>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef unsigned char uchar;

#endif
