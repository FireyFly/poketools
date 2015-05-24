#ifndef POKETOOLS_H
#define POKETOOLS_H

#include <stdint.h>

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef uint8_t  u8;

typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/** Sign-extend `x` using the `b`th bit. */
#define SEXT(x,b) ((!((x) >> (b)) - 1) << (b) | (x))

#endif
