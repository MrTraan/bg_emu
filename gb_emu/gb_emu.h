#pragma once

typedef unsigned char uint8;
typedef unsigned char byte;
typedef char int8;
typedef unsigned short uint16;
typedef short int16;

#define BIT_VALUE(x, n) ((x >> n) & 1)
#define BIT_IS_SET(x, n) ((x >> n) & 1 == 1)
#define BIT_SET(x, n) (x | (1 << n))
#define BIT_UNSET(x, n) (x & ~(1 << n))
