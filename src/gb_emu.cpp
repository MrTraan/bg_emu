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
#include "cpu.h"
#include "memory.h"
#include "rom.h"
#include "gui/window.h"
#include "gui/keyboard.h"
#include "gui/screen_buffer.h"
#include "sound/Gb_Apu.h"
#include "sound/Multi_Buffer.h"
#include "sound/Sound_Queue.h"

void DrawUI();
void DrawDebugWindow();

static Window window;
static MemoryEditor mem_edit;
static Cartridge * cart;
static Memory mem;
Gb_Apu apu;
Stereo_Buffer soundBuffer;
Sound_Queue sound;

static Cpu cpu;
static Ppu ppu;
static bool shouldRun = true;
static bool shouldStep = false;
static int PCBreakpoint = -1;

static void reset( const char * cartridgePath ) {
	if ( cart != nullptr ) {
		delete cart;
	}
	cart = Cartridge::LoadFromFile(cartridgePath);
	mem.cart = cart;
	mem.cpu = &cpu;
	mem.apu = &apu;
	cpu.mem = &mem;
	ppu.mem = &mem;
	ppu.cpu = &cpu;

	Keyboard::s_mem = &mem;
	Keyboard::s_cpu = &cpu;

	soundBuffer.clear();
	apu.reset();
	sound.stop();
	gbemu_assert(sound.start(sample_rate, 2) == nullptr);
	mem.Reset();
	cpu.Reset();
	ppu.Reset();
}

void drop_callback(GLFWwindow * glWindow, int count, const char ** paths) {
	if (count == 1) {
		reset( paths[0] );
	}
}

void windowResizeCallback(GLFWwindow * glWindow, int width, int height) {
	window.Width = width;
	window.Height = height;
	ppu.frontBuffer.RefreshSize( window );
	ppu.backBuffer.RefreshSize( window );
}

std::vector<std::string> romFSPaths;

void parseRomPath( const char * path ) {
	for(const auto & entry : std::filesystem::directory_iterator(path)) {
		if ( entry.is_directory() ) {
			parseRomPath( entry.path().string().c_str() );
		} else {
			std::string str = entry.path().string();
			if (str.find(FS_BASE_PATH "/") == 0) {
				str.erase(0, strlen(FS_BASE_PATH "/"));
			}
			romFSPaths.push_back(str);
		}
	}
}

#if defined( _WIN32 )
void GLAPIENTRY
MessageCallback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
		return;
	char buffer[1000];
	snprintf(buffer, 1000, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
		(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
		type, severity, message);
	OutputDebugString(buffer);
}
#endif

int main(int argc, char **argv)
{
	const char * romPath;
	if (argc == 2) {
		romPath = argv[1];
	} else {
        //romPath = "../../../roms/cpu_instrs.gb";
        romPath = FS_BASE_PATH "/roms/Pokemon_Bleue.gb";
	}
	reset(romPath);
	if (cart == nullptr) {
		return 1;
	}

	parseRomPath( FS_BASE_PATH "/roms" );

	window.Allocate( GB_SCREEN_WIDTH * 8, GB_SCREEN_HEIGHT * 8, "bg_emu" );

	// Setup imgui
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplGlfw_InitForOpenGL(window.GetGlfwWindow(), true);
	ImGui_ImplOpenGL3_Init("#version 150");
	io.Fonts->AddFontFromFileTTF( FS_BASE_PATH "/fonts/consolas.ttf", 13 );

	Keyboard::Init(window);

	int major, minor, version;
	glfwGetVersion(&major, &minor, &version);
	const GLubyte* vendor = glGetString(GL_VENDOR);
	printf("Vendor name: %s\n", vendor);
	printf("Glfw version: %d.%d.%d\n", major, minor, version);
	printf("OpenGL version: %s\n", glGetString(GL_VERSION));
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_DEBUG_OUTPUT);
#if defined( _WIN32 )
	 glDebugMessageCallback(MessageCallback, 0);
#endif
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glfwSetDropCallback(window.GetGlfwWindow(), drop_callback);
	glfwSetWindowSizeCallback(window.GetGlfwWindow(), windowResizeCallback);

	if(SDL_Init(SDL_INIT_AUDIO) < 0)
		return EXIT_FAILURE;
	atexit(SDL_Quit);

	apu.treble_eq(-20.0); // lower values muffle it more
	soundBuffer.bass_freq(461); // higher values simulate smaller speaker
	// Set sample rate and check for out of memory error
	apu.output(soundBuffer.center(), soundBuffer.left(), soundBuffer.right());
	soundBuffer.clock_rate(4194304 * APU_OVERCLOCKING);
	gbemu_assert(soundBuffer.set_sample_rate(sample_rate) == nullptr);

	// Generate a few seconds of sound and play using SDL
	bool show_demo_window = false;

	ppu.AllocateBuffers( window );

	while (!window.ShouldClose()) {
		double startTime = glfwGetTime();

		window.Clear();
		window.PollEvents();
		window.ProcessInput();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (show_demo_window) {
			ImGui::ShowDemoWindow(&show_demo_window);
		}

		ppu.drawingBuffer->Draw();

		DrawUI();

		cpu.cpuTime = 0;
		int maxClocksThisFrame = GBEMU_CLOCK_SPEED / 60;
		if ( Keyboard::IsKeyDown( eKey::KEY_SPACE ) ) {
			maxClocksThisFrame *= 10;
		}
		while (cpu.cpuTime < maxClocksThisFrame && (shouldRun || shouldStep)) {
			int clocks = 4;
			if (!cpu.isOnHalt) {
				clocks = cpu.ExecuteNextOPCode();
			}
			cpu.cpuTime += clocks;
			ppu.Update(clocks);
			cpu.UpdateTimer( clocks );
			cpu.cpuTime += cpu.ProcessInterupts();
			if (PCBreakpoint == cpu.PC) {
				shouldRun = false;
			}
			shouldStep = false;
		}

		int const buf_size = 4096;
		static blip_sample_t buf[buf_size];

		bool stereo = apu.end_frame(cpu.cpuTime * APU_OVERCLOCKING);
		soundBuffer.end_frame(cpu.cpuTime * APU_OVERCLOCKING, stereo);
		if (soundBuffer.samples_avail() >= buf_size){ 
			// Play whatever samples are available
			long count = soundBuffer.read_samples(buf, buf_size);
			sound.write(buf, count);
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		window.SwapBuffers();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	ppu.DestroyBuffers();
	delete cart;
	window.Destroy();
	return 0;
}

void DrawUI() {
	static bool showDebugWindow = true;
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
			if (ImGui::BeginMenu("Open ROM"))
			{
				for ( const auto & str : romFSPaths ) {
					if (ImGui::MenuItem(str.c_str())) {
						std::string strCpy = FS_BASE_PATH "/";
						strCpy += str;
						reset(strCpy.c_str());
					}
				}
				ImGui::EndMenu();
			}
            ImGui::EndMenu();
        }
		if (ImGui::MenuItem(shouldRun ? "Pause" : "Run")) {
			shouldRun = !shouldRun;
		}
		if (ImGui::MenuItem("Reset")) {
			// TODO: Use reset function
			cpu.Reset();
			mem.Reset();
			ppu.Reset();
		}
		if (ImGui::MenuItem("Debug")) {
			showDebugWindow = !showDebugWindow;
		}
		float framerate = ImGui::GetIO().Framerate;
		if ( framerate >= 59.8 && framerate <= 60.0 ) {
			// Avoid text flickering
			framerate = 60.0;
		}
		ImGui::Text("%.1f FPS", framerate);
        ImGui::EndMainMenuBar();
    }
	if ( showDebugWindow ) {
		DrawDebugWindow();
	}
}
void DrawDebugWindow() {
	static float volume = 50.0f;
	if (ImGui::SliderFloat("Volume", &volume, 0.0f, 100.0f)) {
		if (volume > 100.0f) {
			volume = 100.0f;
		}
		apu.volume(volume / 100);
	}
	ImGui::Columns(4, "registers");
	ImGui::Separator();
	ImGui::Text("A"); ImGui::NextColumn();
	ImGui::Text("F"); ImGui::NextColumn();
	ImGui::Text("B"); ImGui::NextColumn();
	ImGui::Text("C"); ImGui::NextColumn();
	ImGui::Separator();
	ImGui::Text("0x%02x", cpu.A.Get()); ImGui::NextColumn();
	ImGui::Text("0x%02x", cpu.F.Get()); ImGui::NextColumn();
	ImGui::Text("0x%02x", cpu.B.Get()); ImGui::NextColumn();
	ImGui::Text("0x%02x", cpu.C.Get()); ImGui::NextColumn();
	ImGui::Columns(2, "registers 16bits");
	ImGui::Separator();
	ImGui::Text("0x%04x", cpu.AF.Get()); ImGui::NextColumn();
	ImGui::Text("0x%04x", cpu.BC.Get()); ImGui::NextColumn();
	ImGui::Columns(4, "registers");
	ImGui::Separator();
	ImGui::Text("D"); ImGui::NextColumn();
	ImGui::Text("E"); ImGui::NextColumn();
	ImGui::Text("H"); ImGui::NextColumn();
	ImGui::Text("L"); ImGui::NextColumn();
	ImGui::Separator();
	ImGui::Text("0x%02x", cpu.D.Get()); ImGui::NextColumn();
	ImGui::Text("0x%02x", cpu.E.Get()); ImGui::NextColumn();
	ImGui::Text("0x%02x", cpu.H.Get()); ImGui::NextColumn();
	ImGui::Text("0x%02x", cpu.L.Get()); ImGui::NextColumn();
	ImGui::Columns(2, "registers 16bits");
	ImGui::Separator();
	ImGui::Text("0x%04x", cpu.DE.Get()); ImGui::NextColumn();
	ImGui::Text("0x%04x", cpu.HL.Get()); ImGui::NextColumn();
	ImGui::Columns(2, "PC and SP");
	ImGui::Separator();
	ImGui::Text("PC"); ImGui::NextColumn();
	ImGui::Text("SP"); ImGui::NextColumn();
	ImGui::Separator();
	ImGui::Text("0x%04x", cpu.PC); ImGui::NextColumn();
	ImGui::Text("0x%04x", cpu.SP.Get()); ImGui::NextColumn();
	ImGui::Columns(4, "flags");
	ImGui::Separator();
	ImGui::Text("Z"); ImGui::NextColumn();
	ImGui::Text("N"); ImGui::NextColumn();
	ImGui::Text("H"); ImGui::NextColumn();
	ImGui::Text("C"); ImGui::NextColumn();
	ImGui::Separator();
	ImGui::Text("%d", cpu.GetZ()); ImGui::NextColumn();
	ImGui::Text("%d", cpu.GetN()); ImGui::NextColumn();
	ImGui::Text("%d", cpu.GetH()); ImGui::NextColumn();
	ImGui::Text("%d", cpu.GetC()); ImGui::NextColumn();

	ImGui::Columns(1);
	ImGui::Separator();
	ImGui::Text("Last instruction: %s", cpu.lastInstructionName);
	ImGui::Text("Next instruction: %s", Cpu::s_instructionsNames[mem.Read(cpu.PC)]);
	ImGui::Checkbox( "Skip bios", &(cpu.skipBios) );

	static char buf[64] = "";
	ImGui::InputText("Break at PC: ", buf, 64, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);
	if (buf[0] != '\0') {
		PCBreakpoint = strtol(buf, nullptr, 16);
	}
	bool shouldStep = false;
	if (ImGui::Button("Step")) {
		shouldStep = true;
	}
	
	static int showRomCode = false;
	if (ImGui::Button(showRomCode ? "Hide ROM Code" : "Show ROM Code")) {
		showRomCode = !showRomCode;
	}
	ImGui::SameLine();
	static int showMemoryInspector = false;
	if (ImGui::Button(showMemoryInspector ? "Hide Memory" : "Show Memory")) {
		showMemoryInspector = !showMemoryInspector;
	}

	if (ImGui::TreeNode("Pixel Processing Unit")) {
		ppu.DebugDraw();
		ImGui::TreePop();
	}
	if ( cart != nullptr ) {
		if (ImGui::TreeNode("Cartridge")) {
			cart->DebugDraw();
			ImGui::TreePop();
		}
	}

	if (showRomCode) {
		ImGui::Begin("ROM Code");
		ImGui::BeginGroup();

		ImGui::BeginChild(ImGui::GetID((void*)(intptr_t)0));
		for (int addr = 0; addr < 0x8000; addr++)
		{
			byte romValue = mem.Read(addr);

			if (romValue == 0) {
				continue; // Displaying a large list cause a huge performance hit, so we might as well not display NOP
			}

			const char * instructionName = Cpu::s_instructionsNames[romValue];
			byte instructionSize = Cpu::s_instructionsSize[romValue];

			bool colored = false;
			if (addr == cpu.PC)
			{
				colored = true;
				ImGui::SetScrollHereY(0.5f); // 0.0f:top, 0.5f:center, 1.0f:bottom
			}
			if (instructionSize == 1) {
				if (colored) {
					ImGui::TextColored(ImVec4(1, 1, 0, 1), "0x%04x %s", addr, instructionName);
				} else {
					ImGui::Text("0x%04x %s", addr, instructionName);
				}
			}
			if (instructionSize == 2) {
				byte arg = mem.Read(addr + 1);
				if (colored) {
					ImGui::TextColored(ImVec4(1, 1, 0, 1), "0x%04x %s %#x", addr, instructionName, arg);
				} else {
					ImGui::Text("0x%04x %s %#x", addr, instructionName, arg);
				}
				addr++;
			}
			if (instructionSize == 3) {
				byte val1 = mem.Read(addr + 1);
				byte val2 = mem.Read(addr + 2);
				uint16 arg = ((uint16)val2 << 8) | val1;
				if (colored) {
					ImGui::TextColored(ImVec4(1, 1, 0, 1), "0x%04x %s %#x", addr, instructionName, arg);
				} else {
					ImGui::Text("0x%04x %s %#x", addr, instructionName, arg);
				}
				addr += 2;
			}
		}
		ImGui::EndChild();
		ImGui::EndGroup();
		ImGui::End();
	}

	if (showMemoryInspector) {
		mem_edit.DrawWindow("VRAM", mem.VRAM, 0x4000, 0x0);
		mem_edit.DrawWindow("HighRAM", mem.highRAM, 0x100, 0x0);
		mem_edit.DrawWindow("OAM", mem.OAM, 0xa0, 0x0);
		mem_edit.DrawWindow("ROM", mem.cart->GetRawMemory(), mem.cart->GetRawMemorySize(), 0x0);
	}
}
