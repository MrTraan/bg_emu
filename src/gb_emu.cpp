#include <stdio.h>
#include <thread>
#include <chrono>
#include <iostream>
#include <GL/gl3w.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_memory_editor.h>
#include "SDL.h"
#if defined (_WIN32)
#include <filesystem>
#elif defined( __linux ) || defined( __APPLE__ )
#include <sys/types.h>
#include <dirent.h>
#else
GBEMU_UNSUPPORTED_PLATFORM
#endif

#include "gb_emu.h"
#include "gameboy.h"
#include "cpu.h"
#include "rom.h"
#include "gui/window.h"
#include "sound/Gb_Apu.h"
#include "sound/Multi_Buffer.h"
#include "sound/Sound_Queue.h"
#include "sys.h"

void DrawUI();

static Window	window;
static Gameboy	gb;

std::vector< std::string > romFSPaths;

void parseRomPath( const char * path ) {
#if defined ( _WIN32 )
	for ( const auto & entry : std::filesystem::directory_iterator( path ) ) {
		if ( entry.is_directory() ) {
			parseRomPath( entry.path().string().c_str() );
		} else {
			std::string str = entry.path().string();
			if ( str.find( FS_BASE_PATH "/" ) == 0 ) {
				str.erase( 0, strlen( FS_BASE_PATH "/" ) );
			}
			romFSPaths.push_back( str );
		}
	}
#elif defined( __linux ) || defined( __APPLE__ )
	DIR * dir = opendir( path );
	if ( dir == nullptr ) {
		return;
	}

	dirent * dirFiles;
	while ( ( dirFiles = readdir( dir) ) != nullptr ) {
		if (strcmp(dirFiles->d_name, ".") == 0 || strcmp(dirFiles->d_name, "..") == 0)
                continue;
		if ( dirFiles->d_type == DT_DIR ) {
			parseRomPath( dirFiles->d_name );
		} else {
			std::string str = path;
			str += "/";
			str += dirFiles->d_name;
			if ( str.find( FS_BASE_PATH "/" ) == 0 ) {
				str.erase( 0, strlen( FS_BASE_PATH "/" ) );
			}
			romFSPaths.push_back( str );
		}
	}
	closedir( dir );
#else
GBEMU_UNSUPPORTED_PLATFORM
#endif
}

int main( int argc, char ** argv ) {
	InitSys();

	const char * romPath;
	if ( argc == 2 ) {
		romPath = argv[ 1 ];
	} else {
		// romPath = "../../../roms/cpu_instrs.gb";
		romPath = FS_BASE_PATH "/roms/Pokemon - Jaune.gbc";
	}
	gb.LoadCart( romPath );
	if ( gb.cart == nullptr ) {
		return 1;
	}
	
	if ( SDL_Init( SDL_INIT_AUDIO | SDL_INIT_VIDEO ) < 0 )
		return EXIT_FAILURE;
	atexit( SDL_Quit );

	parseRomPath( FS_BASE_PATH "/roms" );

	window.Allocate( GB_SCREEN_WIDTH * 8, GB_SCREEN_HEIGHT * 8, "bg_emu" );

	// Setup imgui
	ImGui::CreateContext();
	ImGuiIO & io = ImGui::GetIO();
	ImGui_ImplSDL2_InitForOpenGL( window.glWindow, window.glContext );
	ImGui_ImplOpenGL3_Init( "#version 150" );
	io.Fonts->AddFontFromFileTTF( FS_BASE_PATH "/fonts/consolas.ttf", 13 );

	const GLubyte * vendor = glGetString( GL_VENDOR );
	printf( "Vendor name: %s\n", vendor );
	printf( "OpenGL version: %s\n", glGetString( GL_VERSION ) );
	glEnable( GL_MULTISAMPLE );
	glEnable( GL_DEBUG_OUTPUT );
	glEnable( GL_CULL_FACE );
	glCullFace( GL_BACK );

	gb.apu.treble_eq( -20.0 );		// lower values muffle it more
	gb.soundBuffer.bass_freq( 461 );	// higher values simulate smaller speaker
	// Set sample rate and check for out of memory error
	gb.apu.output( gb.soundBuffer.center(), gb.soundBuffer.left(), gb.soundBuffer.right() );
	gb.soundBuffer.clock_rate( 4194304 * APU_OVERCLOCKING );
	gbemu_assert( gb.soundBuffer.set_sample_rate( sample_rate ) == nullptr );

	// Generate a few seconds of sound and play using SDL
	bool show_demo_window = true;

	gb.ppu.AllocateBuffers( window );

	while ( !window.ShouldClose() ) {
		window.Clear();
		window.PollEvents( &gb );
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame( window.glWindow );
		ImGui::NewFrame();

		if ( show_demo_window ) {
			ImGui::ShowDemoWindow( &show_demo_window );
		}

		DrawUI();

		gb.RunOneFrame();

		{
			int const				buf_size = 4096;
			static blip_sample_t	buf[ buf_size ];

			bool stereo = gb.apu.end_frame( gb.cpu.cpuTime * APU_OVERCLOCKING );
			gb.soundBuffer.end_frame( gb.cpu.cpuTime * APU_OVERCLOCKING, stereo );
			if ( gb.soundBuffer.samples_avail() >= buf_size ) {
				// Play whatever samples are available
				long count = gb.soundBuffer.read_samples( buf, buf_size );
				gb.sound.write( buf, count );
			}
		}
		gb.ppu.drawingBuffer->Draw();
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );
		SDL_GL_SwapWindow(window.glWindow);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	gb.ppu.DestroyBuffers();
	delete gb.cart;
	window.Destroy();
	return 0;
}

void DrawUI() {
	static bool showDebugWindow = true;
	if ( ImGui::BeginMainMenuBar() ) {
		if ( ImGui::BeginMenu( "File" ) ) {
			if ( ImGui::BeginMenu( "Open ROM" ) ) {
				for ( const auto & str : romFSPaths ) {
					if ( ImGui::MenuItem( str.c_str() ) ) {
						std::string strCpy = FS_BASE_PATH "/";
						strCpy += str;
						gb.LoadCart( strCpy.c_str() );
					}
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		if ( gb.cart != nullptr ) {
			if ( ImGui::BeginMenu( "Save state" ) ) {
				char fileName[ 200 ];
				strncpy( fileName, gb.cart->romName, 200 );
				strncat( fileName, ".save_state", 200 );
				if ( ImGui::MenuItem( "Save" ) ) {
					gb.SerializeSaveState( fileName );
				}
				if ( ImGui::MenuItem( "Load" ) ) {
					gb.LoadSaveState( fileName );
				}
				ImGui::EndMenu();
			}
		}
		if ( ImGui::MenuItem( gb.shouldRun ? "Pause" : "Run" ) ) {
			gb.shouldRun = !gb.shouldRun;
		}
		if ( ImGui::MenuItem( "Reset" ) ) {
			gb.Reset();
		}
		if ( ImGui::MenuItem( "Debug" ) ) {
			showDebugWindow = !showDebugWindow;
		}
		float framerate = ImGui::GetIO().Framerate;
		if ( framerate >= 59.8 && framerate <= 60.0 ) {
			// Avoid text flickering
			framerate = 60.0;
		}
		ImGui::Text( "%.1f FPS", framerate );
		ImGui::EndMainMenuBar();
	}

	if ( showDebugWindow ) {
		gb.DebugDraw();
	}
}
