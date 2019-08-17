#pragma once

typedef unsigned char uint8;
typedef unsigned char byte;
typedef char int8;
typedef unsigned short uint16;
typedef short int16;

#define BIT_VALUE(x, n) ((x >> n) & 1)
#define BIT_IS_SET(x, n) (((x >> n) & 1) == 1)
#define BIT_SET(x, n) (x | (1 << n))
#define BIT_UNSET(x, n) (x & ~(1 << n))

#define GBEMU_UNSUPPORTED_PLATFORM static_assert(false, "Platform specific not handled here");


// define the debug break by platform
#if defined( _WIN32 )
#define DEBUG_BREAK __debugbreak()
#elif defined( __linux )
#define DEBUG_BREAK __asm__ __volatile__ ("int $0x03")
#elif defined ( __APPLE__ )
#include <signal.h>
#define DEBUG_BREAK raise(SIGTRAP)
#else
GBEMU_UNSUPPORTED_PLATFORM
#endif
