#include <stdio.h>
#include <thread>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_memory_editor.h>
#include "SDL.h"

#include "gb_emu.h"
#include "gameboy.h"
#include "cpu.h"
#include "rom.h"
#include "gui/window.h"
#include "gui/keyboard.h"
#include "sound/Gb_Apu.h"
#include "sound/Multi_Buffer.h"
#include "sound/Sound_Queue.h"

void DrawUI();

static Window	window;
static Gameboy	gb;
Stereo_Buffer	soundBuffer;
Sound_Queue		sound;

static void reset( const char * cartridgePath ) {
	gb.LoadCart( cartridgePath );

	soundBuffer.clear();
	sound.stop();
	gbemu_assert( sound.start( sample_rate, 2 ) == nullptr );
}

void drop_callback( GLFWwindow * glWindow, int count, const char ** paths ) {
	if ( count == 1 ) {
		reset( paths[ 0 ] );
	}
}

void windowResizeCallback( GLFWwindow * glWindow, int width, int height ) {
	window.Width = width;
	window.Height = height;
	gb.ppu.frontBuffer.RefreshSize( window );
	gb.ppu.backBuffer.RefreshSize( window );
}

std::vector< std::string > romFSPaths;

void parseRomPath( const char * path ) {
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
}

#if defined( _WIN32 )
void GLAPIENTRY MessageCallback( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar * message, const void * userParam ) {
	if ( severity == GL_DEBUG_SEVERITY_NOTIFICATION )
		return;
	char buffer[ 1000 ];
	snprintf( buffer, 1000, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n", ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ), type, severity, message );
	OutputDebugString( buffer );
}
#endif

int main( int argc, char ** argv ) {
	const char * romPath;
	if ( argc == 2 ) {
		romPath = argv[ 1 ];
	} else {
		// romPath = "../../../roms/cpu_instrs.gb";
		romPath = FS_BASE_PATH "/roms/Pokemon - Jaune.gbc";
	}
	reset( romPath );
	if ( gb.cart == nullptr ) {
		return 1;
	}

	parseRomPath( FS_BASE_PATH "/roms" );

	window.Allocate( GB_SCREEN_WIDTH * 8, GB_SCREEN_HEIGHT * 8, "bg_emu" );

	// Setup imgui
	ImGui::CreateContext();
	ImGuiIO & io = ImGui::GetIO();
	ImGui_ImplGlfw_InitForOpenGL( window.GetGlfwWindow(), true );
	ImGui_ImplOpenGL3_Init( "#version 150" );
	io.Fonts->AddFontFromFileTTF( FS_BASE_PATH "/fonts/consolas.ttf", 13 );

	Keyboard::s_gb = &gb;
	Keyboard::Init( window );

	int major, minor, version;
	glfwGetVersion( &major, &minor, &version );
	const GLubyte * vendor = glGetString( GL_VENDOR );
	printf( "Vendor name: %s\n", vendor );
	printf( "Glfw version: %d.%d.%d\n", major, minor, version );
	printf( "OpenGL version: %s\n", glGetString( GL_VERSION ) );
	glEnable( GL_MULTISAMPLE );
	glEnable( GL_DEBUG_OUTPUT );
#if defined( _WIN32 )
	glDebugMessageCallback( MessageCallback, 0 );
#endif
	glEnable( GL_CULL_FACE );
	glCullFace( GL_BACK );
	glfwSetDropCallback( window.GetGlfwWindow(), drop_callback );
	glfwSetWindowSizeCallback( window.GetGlfwWindow(), windowResizeCallback );

	if ( SDL_Init( SDL_INIT_AUDIO ) < 0 )
		return EXIT_FAILURE;
	atexit( SDL_Quit );

	gb.apu.treble_eq( -20.0 );		// lower values muffle it more
	soundBuffer.bass_freq( 461 );	// higher values simulate smaller speaker
	// Set sample rate and check for out of memory error
	gb.apu.output( soundBuffer.center(), soundBuffer.left(), soundBuffer.right() );
	soundBuffer.clock_rate( 4194304 * APU_OVERCLOCKING );
	gbemu_assert( soundBuffer.set_sample_rate( sample_rate ) == nullptr );

	// Generate a few seconds of sound and play using SDL
	bool show_demo_window = true;

	gb.ppu.AllocateBuffers( window );

	while ( !window.ShouldClose() ) {
		double startTime = glfwGetTime();

		window.Clear();
		window.PollEvents();
		window.ProcessInput();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if ( show_demo_window ) {
			ImGui::ShowDemoWindow( &show_demo_window );
		}

		gb.ppu.drawingBuffer->Draw();

		DrawUI();

		gb.RunOneFrame();

		int const				buf_size = 4096;
		static blip_sample_t	buf[ buf_size ];

		bool stereo = gb.apu.end_frame( gb.cpu.cpuTime * APU_OVERCLOCKING );
		soundBuffer.end_frame( gb.cpu.cpuTime * APU_OVERCLOCKING, stereo );
		if ( soundBuffer.samples_avail() >= buf_size ) {
			// Play whatever samples are available
			long count = soundBuffer.read_samples( buf, buf_size );
			sound.write( buf, count );
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );

		window.SwapBuffers();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
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
						reset( strCpy.c_str() );
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
