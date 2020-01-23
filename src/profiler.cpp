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
	frames.push_back( currentFrame );
	if ( currentFrame.totalTime > frameTooLongThreshold ) {
		abnormalFrames.push_back( &frames.back() );
	}
	currentFrame.Reset();
	currentFrame.label = "Frame";
	currentParentPtr = &currentFrame;
}

void Profiler::StartProfilingMark( const ProfilerMark * mark, const char * tag ) {
	if ( !isActive ) {
		return;
	}
	FrameInfo newFrame;
	
	newFrame.parent = currentParentPtr;
	newFrame.label = tag;
	currentParentPtr->children.push_back( newFrame );
	currentParentPtr = &currentParentPtr->children.back();
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

	if ( frames.size() > 1 ) {
		ImGui::InputInt( "Selected frame", &frameCursor );
		if ( ImGui::Button( "Jump to last" ) ) {
			frameCursor = frames.size() - 1;
		}
		if ( frameCursor < 1 )
			frameCursor = 1;
		if ( frameCursor >= frames.size() )
			frameCursor = frames.size() - 1;

		auto const & frame = frames[frameCursor];
		ImGui::Text("Total frame duration: %fms", frame.totalTime);
		DrawOneFrameInfo( frame );
		
		ImGui::Text("%d frames longer than %fms", abnormalFrames.size(), frameTooLongThreshold / 1000);
		if ( abnormalFrames.size() > 0 ) {
			if ( ImGui::TreeNode( "Show last slow frame" ) ) {
				auto lastSlowFrame = abnormalFrames.back();
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
	std::map< const char *, totalTimeDetails > totalTime;

	char label[ 100 ];
	for ( const auto & child : info.children ) {
		if ( totalTime.find( child.label ) != totalTime.end() ) {
			auto & e = totalTime[ child.label ];
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
			totalTime[ child.label ] = e;
		}
	}

	ImGui::Text("Total time:");
	for ( std::pair< const char *, totalTimeDetails > elem : totalTime ) {
		ImGui::Text( "%s: count %d, total: %fms, best: %fms, worst: %fms", elem.first, elem.second.count, elem.second.total / 1000, elem.second.best / 1000, elem.second.worst / 1000 );
	}

	for ( const auto & child : info.children ) {
		snprintf( label, 100, "%s: %fms", child.label, child.totalTime / 1000 );
		if ( child.children.size() > 0 ) {
			if ( ImGui::TreeNode( label ) ) {
				DrawOneFrameInfo( child );
				ImGui::TreePop();
			}
		} else {
			ImGui::Text( "   %s", label );
		}
	}
}
