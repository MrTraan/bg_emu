#include <stdio.h>
#include <thread>
#include <chrono>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include "gb_emu.h"
#include "cpu.h"
#include "memory.h"
#include "gui/window.h"
#include "gui/keyboard.h"
#include "gui/screen_buffer.h"

void DrawDebugWindow(Cpu& cpu, Memory& mem);

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

int main()
{
	Memory * mem = new Memory();
	Cpu cpu(mem);
	Window window;
	ScreenBuffer screenBuffer;

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


	bool show_demo_window = true;

	while (!window.ShouldClose()) {
		double startTime = glfwGetTime();

		window.Clear();
		window.PollEvents();
		window.ProcessInput();
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		
		screenBuffer.Draw();

		DrawDebugWindow(cpu, *mem);
		//int totalClocksThisFrame = 0;
		//while (totalClocksThisFrame < GBEMU_CLOCK_SPEED / 60) {
		//	totalClocksThisFrame += cpu.ExecuteNextOPCode();
		//}
		if (Keyboard::IsKeyPressed(eKey::KEY_SPACE)) {
			cpu.ExecuteNextOPCode();
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		window.SwapBuffers();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	delete mem;
	return 0;
}

void DrawDebugWindow(Cpu& cpu, Memory& mem) {
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

	ImGui::Columns(1);
	ImGui::Separator();
}
