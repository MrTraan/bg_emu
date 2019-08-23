#include <stdio.h>
#include <thread>
#include <chrono>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_memory_editor.h>

#include "gb_emu.h"
#include "cpu.h"
#include "memory.h"
#include "rom.h"
#include "gui/window.h"
#include "gui/keyboard.h"
#include "gui/screen_buffer.h"

void DrawDebugWindow(Cpu& cpu, Memory& mem, bool showRomCode);

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
	fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
		(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
		type, severity, message);
}

static MemoryEditor mem_edit;
static Cartridge * cart;
static Memory * mem;
static Cpu * cpu;
static Ppu * ppu;
static bool shouldRun = false;

void drop_callback(GLFWwindow * window, int count, const char ** paths) {
	if (count == 1) {
		delete mem;
		delete cart;
		delete cpu;
		delete ppu;

		cart = Cartridge::LoadFromFile(paths[0]);
		mem = new Memory(cart);
		ppu = new Ppu(mem, cpu);

		Keyboard::s_mem = mem;
		Keyboard::s_cpu = cpu;
		cpu->mem = mem;
		cpu->Reset();
		ppu->Reset();
		shouldRun = false;
	}
}

int main(int argc, char **argv)
{
	const char * romPath;
	if (argc == 2) {
		romPath = argv[1];
	} else {
        //romPath = "../../../roms/cpu_instrs.gb";
        romPath = "../../../roms/tetris.gb";
	}
	cart = Cartridge::LoadFromFile(romPath);
	if (cart == nullptr) {
		return 1;
	}
	
	Window window;

	mem = new Memory(cart);
	cpu = new Cpu(mem);
	ppu = new Ppu(mem, cpu);

	// Setup imgui
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	ImGui_ImplGlfw_InitForOpenGL(window.GetGlfwWindow(), true);
	ImGui_ImplOpenGL3_Init("#version 150");

	Keyboard::Init(window);
	Keyboard::s_mem = mem;
	Keyboard::s_cpu = cpu;

	int major, minor, version;
	glfwGetVersion(&major, &minor, &version);
	const GLubyte* vendor = glGetString(GL_VENDOR);
	printf("Vendor name: %s\n", vendor);
	std::string sVendor = (char*)vendor;
	std::string sNvidia = "NVIDIA";
	bool isGraphicCardNvidia = sVendor.find(sNvidia) != std::string::npos;
	printf("Glfw version: %d.%d.%d\n", major, minor, version);
	printf("OpenGL version: %s\n", glGetString(GL_VERSION));
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glfwSetDropCallback(window.GetGlfwWindow(), drop_callback);


	bool show_demo_window = true;

	unsigned long PCBreakpoint = 0x0;
	int num_instructions = 0;

	shouldRun = false; 
	bool showRomCode = false;
	while (!window.ShouldClose()) {
		double startTime = glfwGetTime();

		window.Clear();
		window.PollEvents();
		window.ProcessInput();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		//if (show_demo_window) {
		//	ImGui::ShowDemoWindow(&show_demo_window);
		//}
		
		ppu->frontBuffer->Draw();

		DrawDebugWindow(*cpu, *mem, showRomCode);

		static char buf[64] = "";
		ImGui::Text("Num instructions: %d", num_instructions);
		ImGui::InputText("Break at PC: ", buf, 64, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);
		PCBreakpoint = strtoul(buf, nullptr, 16);
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
			cpu->Reset();
			mem->Reset();
			ppu->Reset();
		}
        if (ImGui::Button(showRomCode ? "Hide ROM Code" : "Show ROM Code")) {
			showRomCode = !showRomCode;
		}
        
		ImGui::Image((void*)(ppu->frontBuffer->textureHandler), ImVec2(GB_SCREEN_WIDTH, GB_SCREEN_HEIGHT), ImVec2(0,0), ImVec2(1,1), ImVec4(1.0f,1.0f,1.0f,1.0f), ImVec4(1.0f,1.0f,1.0f,0.5f));

		int totalClocksThisFrame = 0;
		while (totalClocksThisFrame < GBEMU_CLOCK_SPEED / 60 && (shouldRun || shouldStep)) {
			int clocks = cpu->ExecuteNextOPCode();
			num_instructions++;
			totalClocksThisFrame += clocks;
			ppu->Update(clocks);
			totalClocksThisFrame += cpu->ProcessInterupts();
			if (PCBreakpoint == cpu->PC) {
				shouldRun = false;
			}
			shouldStep = false;
		}
            

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		window.SwapBuffers();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	delete cart;
	delete mem;
	delete ppu;
	delete cpu;
	return 0;
}

void DrawDebugWindow(Cpu& cpu, Memory& mem, bool showRomCode) {
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


	mem_edit.DrawWindow("VRAM", mem.VRAM, 0x4000, 0x0);
	mem_edit.DrawWindow("HighRAM", mem.highRAM, 0x100, 0x0);
	mem_edit.DrawWindow("OAM", mem.OAM, 0xa0, 0x0);
	mem_edit.DrawWindow("ROM", mem.cart->GetRawMemory(), mem.cart->GetRawMemorySize(), 0x0);
}
