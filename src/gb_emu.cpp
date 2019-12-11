#include <stdio.h>
#include <thread>
#include <chrono>
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

void DrawDebugWindow();

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

void drop_callback(GLFWwindow * window, int count, const char ** paths) {
	if (count == 1) {
		reset( paths[0] );
	}
}

int main(int argc, char **argv)
{
	const char * romPath;
	if (argc == 2) {
		romPath = argv[1];
	} else {
        //romPath = "../../../roms/cpu_instrs.gb";
        romPath = "../roms/tetris.gb";
	}
	reset(romPath);
	if (cart == nullptr) {
		return 1;
	}

	Window window;

	// Setup imgui
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	ImGui_ImplGlfw_InitForOpenGL(window.GetGlfwWindow(), true);
	ImGui_ImplOpenGL3_Init("#version 150");

	Keyboard::Init(window);

	int major, minor, version;
	glfwGetVersion(&major, &minor, &version);
	const GLubyte* vendor = glGetString(GL_VENDOR);
	printf("Vendor name: %s\n", vendor);
	printf("Glfw version: %d.%d.%d\n", major, minor, version);
	printf("OpenGL version: %s\n", glGetString(GL_VERSION));
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_DEBUG_OUTPUT);
	// glDebugMessageCallback(MessageCallback, 0);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glfwSetDropCallback(window.GetGlfwWindow(), drop_callback);


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
	bool show_demo_window = true;

	ppu.AllocateBuffers();

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

		ppu.frontBuffer->Draw();

		DrawDebugWindow();

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
	return 0;
}

void DrawDebugWindow() {
	static float volume = 50.0f;
	if (ImGui::SliderFloat("Volume", &volume, 0.0f, 100.0f)) {
		if (volume > 100.0f) {
			volume = 100.0f;
		}
		apu.volume(volume / 100);
	}
	ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
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
	if (ImGui::Button(shouldRun ? "Pause" : "Run")) {
		shouldRun = !shouldRun;
	}
	bool shouldStep = false;
	ImGui::SameLine();
	if (ImGui::Button("Step")) {
		shouldStep = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Reset")) {
		cpu.Reset();
		mem.Reset();
		ppu.Reset();
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

	if (showRomCode) {
		ImGui::Begin("ROM Code");
		ImGui::BeginGroup();

		ImGuiWindowFlags child_flags = false;
		ImGui::BeginChild(ImGui::GetID((void*)(intptr_t)0));
		if (ImGui::BeginMenuBar())
		{
			ImGui::TextUnformatted("abc");
			ImGui::EndMenuBar();
		}
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
