#pragma once

#include <time.h>
#include <chrono>
#include <vector>
#include "gb_emu.h"

typedef std::chrono::time_point< std::chrono::steady_clock > hr_clock;

#define HR_NOW() std::chrono::high_resolution_clock::now()

#define __CONCAT_LABELS( a, b ) a ## b
#define CONCAT_LABELS(a, b) __CONCAT_LABELS(a, b)

#define CREATE_PROFILE_MARK_NAME CONCAT_LABELS(profileMark, __LINE__)

// TODO: remove in release
#define GB_PROFILE( label ) ProfilerMark CREATE_PROFILE_MARK_NAME( FnvHash( #label ), #label )


constexpr double frameTooLongThreshold = 20000;

struct ProfilerMark;

struct Profiler {
	void ImplNewFrame();

	void StartProfilingMark( const ProfilerMark * mark, const char * tag );
	void StopProfilingMark( const ProfilerMark * mark );

	void Draw();

	static Profiler * GrabInstance();

	struct FrameInfo {
		void Reset() { label = nullptr; totalTime = 0; children.clear(); }
		double totalTime;

		const char * label;
		std::vector< FrameInfo > children;
		FrameInfo * parent = nullptr;
	};
	
	void DrawOneFrameInfo( const FrameInfo & info );

	bool isActive = false;

	FrameInfo currentFrame;
	FrameInfo * currentParentPtr = nullptr;
	std::vector< FrameInfo > frames;

	std::vector< FrameInfo * > abnormalFrames;

	hr_clock frameStartTime;
};

struct ProfilerMark {
	ProfilerMark( uint32 tagHash, const char * tag ) {
		startTime = HR_NOW();
		Profiler * prof = Profiler::GrabInstance();
		prof->StartProfilingMark( this, tag );
	}

	~ProfilerMark() {
		endTime = HR_NOW();
		Profiler * prof = Profiler::GrabInstance();
		prof->StopProfilingMark( this );
	}

	hr_clock startTime;
	hr_clock endTime;
};
