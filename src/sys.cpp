#include "sys.h"

#if defined( _WIN32 )
#include <Windows.h>
#endif

static int64 clockTicksPerSecond = 0;
static int64 clockTicksAtStartup = 0;

void InitSys() {
#if defined( _WIN32 )
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	clockTicksPerSecond = li.QuadPart;
	QueryPerformanceCounter(&li);
	clockTicksAtStartup = li.QuadPart;
#elif defined( __linux )
GBEMU_UNSUPPORTED_PLATFORM
#elif defined( __APPLE__ )
GBEMU_UNSUPPORTED_PLATFORM
#else
GBEMU_UNSUPPORTED_PLATFORM
#endif
}

float SysGetTimeInMs() {
#if defined( _WIN32 )
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	int64 ticks = li.QuadPart - clockTicksAtStartup;
	return static_cast<float>( ticks* 1000 ) / static_cast<float>( clockTicksPerSecond );
#elif defined( __linux )
GBEMU_UNSUPPORTED_PLATFORM
#elif defined( __APPLE__ )
GBEMU_UNSUPPORTED_PLATFORM
#else
GBEMU_UNSUPPORTED_PLATFORM
#endif
}
