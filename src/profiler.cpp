#include <map>
#include "profiler.h"
#include "imgui/imgui.h"

Profiler * Profiler::GrabInstance() {
	static Profiler instance;
	return &instance;
}

void Profiler::ImplNewFrame() {
	if ( !isActive ) {
		return;
	}

	currentFrame.totalTime = std::chrono::duration< double, std::micro >( HR_NOW() - frameStartTime ).count();
	frameStartTime = HR_NOW();
	frames.PushBack( currentFrame );
	if ( currentFrame.totalTime > frameTooLongThreshold ) {
		abnormalFrames.PushBack( &frames.Last() );
	}
	currentFrame.Reset();
	currentFrame.label = "Frame";
	currentParentPtr = &currentFrame;
}

void Profiler::StartProfilingMark( const ProfilerMark * mark, uint64 hash, const char * tag ) {
	if ( !isActive ) {
		return;
	}
	FrameInfo & newFrame = currentParentPtr->children.AllocateOne();
	
	newFrame.parent = currentParentPtr;
	newFrame.label = tag;
	newFrame.hash = hash;
	currentParentPtr = &newFrame;

	hashToString[ hash ] = tag;
}

void Profiler::StopProfilingMark( const ProfilerMark * mark ) {
	if ( !isActive ) {
		return;
	}
	currentParentPtr->totalTime = std::chrono::duration< double, std::micro >( mark->endTime - mark->startTime ).count();
	currentParentPtr = currentParentPtr->parent;
}

void Profiler::Draw() {
	static int frameCursor = 0;

	if ( ImGui::Button( isActive ? "Stop" : "Start" ) ) {
		isActive = !isActive;
		if ( isActive ) {
			ImplNewFrame();
		}
	}

	if ( frames.count > 1 ) {
		ImGui::InputInt( "Selected frame", &frameCursor );
		if ( ImGui::Button( "Jump to last" ) ) {
			frameCursor = frames.count - 1;
		}
		if ( frameCursor < 1 )
			frameCursor = 1;
		if ( frameCursor >= frames.count )
			frameCursor = frames.count - 1;

		auto const & frame = frames.At(frameCursor);
		ImGui::Text("Total frame duration: %fms", frame.totalTime);
		DrawOneFrameInfo( frame );
		
		ImGui::Text("%d frames longer than %fms", abnormalFrames.count, frameTooLongThreshold / 1000);
		if ( abnormalFrames.count > 0 ) {
			if ( ImGui::TreeNode( "Show last slow frame" ) ) {
				auto lastSlowFrame = abnormalFrames.Last();
				DrawOneFrameInfo( *lastSlowFrame );
				ImGui::TreePop();
			}
		}
	}
}

void Profiler::DrawOneFrameInfo( const FrameInfo & info ) {
	struct totalTimeDetails {
		int count;
		double total;
		double best;
		double worst;
		double average;
	};
	std::map< uint64, totalTimeDetails > totalTime;

	for ( const auto & child : info.children ) {
		if ( totalTime.find( child.hash ) != totalTime.end() ) {
			auto & e = totalTime[ child.hash ];
			e.total += child.totalTime;
			e.count++;
			if ( child.totalTime > e.worst ) {
				e.worst = child.totalTime;
			}
			if ( child.totalTime < e.best ) {
				e.best = child.totalTime;
			}
		} else {
			totalTimeDetails e;
			e.count = 1;
			e.total = child.totalTime;
			e.best = e.total;
			e.worst = e.total;
			e.average = e.total;
			totalTime[ child.hash ] = e;
		}
	}

	ImGui::Text("Total time:");
	for ( std::pair< uint64, totalTimeDetails > elem : totalTime ) {
		ImGui::Text( "%s: count %d, total: %fms, best: %fms, worst: %fms", hashToString[ elem.first ], elem.second.count, elem.second.total / 1000, elem.second.best / 1000, elem.second.worst / 1000 );
	}

	char label[ 100 ];
	for ( const auto & child : info.children ) {
		snprintf( label, 100, "%s: %fms", child.label, child.totalTime / 1000 );
		if ( child.children.count > 0 ) {
			if ( ImGui::TreeNode( label ) ) {
				DrawOneFrameInfo( child );
				ImGui::TreePop();
			}
		} else {
			ImGui::Text( "   %s", label );
		}
	}
}
