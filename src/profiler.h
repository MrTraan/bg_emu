#pragma once

#include <time.h>
#include <chrono>
#include <map>
#include "containers.h"
#include "gb_emu.h"

typedef std::chrono::time_point< std::chrono::steady_clock > hr_clock;

#define HR_NOW() std::chrono::high_resolution_clock::now()

#define __CONCAT_LABELS( a, b ) a ## b
#define CONCAT_LABELS(a, b) __CONCAT_LABELS(a, b)

#define CREATE_PROFILE_MARK_NAME CONCAT_LABELS(profileMark, __LINE__)

// TODO: remove in release
#define GB_PROFILE( label ) ProfilerMark CREATE_PROFILE_MARK_NAME( COMPILE_TIME_HASH( #label ), #label )

constexpr double frameTooLongThreshold = 20000;

struct ProfilerMark;

struct Profiler {
	void ImplNewFrame();

	void StartProfilingMark( const ProfilerMark * mark, uint64 hash, const char * tag );
	void StopProfilingMark( const ProfilerMark * mark );

	void Draw();

	static Profiler * GrabInstance();

	struct FrameInfo {
		void Reset() { label = nullptr; totalTime = 0; children.Clear(); }
		double totalTime;

		const char * label = nullptr;
		uint64 hash = 0;
		DynamicArray< FrameInfo > children;
		FrameInfo * parent = nullptr;
	};
	
	void DrawOneFrameInfo( const FrameInfo & info );

	bool isActive = false;

	FrameInfo currentFrame;
	FrameInfo * currentParentPtr = nullptr;
	DynamicArray< FrameInfo > frames;

	DynamicArray< FrameInfo * > abnormalFrames;

	hr_clock frameStartTime;

	std::map< uint64, const char * > hashToString;
};

struct ProfilerMark {
	ProfilerMark( uint64 tagHash, const char * tag ) {
		Profiler * prof = Profiler::GrabInstance();
		if ( prof->isActive == false )
			return;
		startTime = HR_NOW();
		prof->StartProfilingMark( this, tagHash, tag );
	}

	~ProfilerMark() {
		Profiler * prof = Profiler::GrabInstance();
		if ( prof->isActive == false )
			return;
		endTime = HR_NOW();
		prof->StopProfilingMark( this );
	}

	hr_clock startTime;
	hr_clock endTime;
};
