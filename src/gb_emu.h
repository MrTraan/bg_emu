#pragma once
#include <stdio.h>

typedef unsigned char	uint8;
typedef unsigned char	byte;
typedef char			int8;
typedef unsigned short	uint16;
typedef short			int16;
typedef unsigned int	uint32;
typedef int				int32;

#define BIT_VALUE( x, n )  ( ( x >> n ) & 1 )
#define BIT_IS_SET( x, n ) ( ( ( x >> n ) & 1 ) == 1 )
#define BIT_SET( x, n )	   ( x | ( 1 << n ) )
#define BIT_UNSET( x, n )  ( x & ~( 1 << n ) )
#define BIT_HIGH_8( x )	   ( ( byte )( ( x & 0xFF00 ) >> 8 ) )
#define BIT_LOW_8( x )	   ( ( byte )( x & 0xFF ) )
#define MIN( x, y )		   ( x < y ? x : y )
#define MAX( x, y )		   ( x >= y ? x : y )

#define GBEMU_UNSUPPORTED_PLATFORM static_assert( false, "Platform specific not handled here" );

// define the debug break by platform
#if defined( _WIN32 )
#define DEBUG_BREAK __debugbreak()
#elif defined( __linux )
#define DEBUG_BREAK __asm__ __volatile__( "int $0x03" )
#elif defined( __APPLE__ )
#include <signal.h>
#define DEBUG_BREAK raise( SIGTRAP )
#else
GBEMU_UNSUPPORTED_PLATFORM
#endif

// GameBoy has a 4.19 MHz CPU
constexpr int	GBEMU_CLOCK_SPEED = 4194304;
constexpr int	APU_OVERCLOCKING = 1;
constexpr long	sample_rate = 44100;

constexpr int GB_SCREEN_WIDTH = 160;
constexpr int GB_SCREEN_HEIGHT = 144;

#define gbemu_assert( x )                                                                                                                                      \
	if ( !( x ) ) {                                                                                                                                            \
		fprintf( stderr, "ASSERTION FAILED: " #x "\n" );                                                                                                       \
		DEBUG_BREAK;                                                                                                                                           \
	}

struct Pixel {
	byte R;
	byte G;
	byte B;
};
