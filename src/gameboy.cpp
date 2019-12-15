#include <stdlib.h>
#include "gameboy.h"
#include <imgui/imgui.h>
#include <imgui/imgui_memory_editor.h>

static MemoryEditor mem_edit;

void Gameboy::RunOneFrame() {
	cpu.cpuTime = 0;
	int maxClocksThisFrame = GBEMU_CLOCK_SPEED / 60;
	//if ( Keyboard::IsKeyDown( eKey::KEY_SPACE ) ) {
	//	maxClocksThisFrame *= 10;
	//}
	while (cpu.cpuTime < maxClocksThisFrame && (shouldRun || shouldStep)) {
		int clocks = 4;
		if (!cpu.isOnHalt) {
			clocks = cpu.ExecuteNextOPCode( this );
		}
		cpu.cpuTime += clocks;
		ppu.Update(clocks, this);
		cpu.UpdateTimer( clocks, this );
		cpu.cpuTime += cpu.ProcessInterupts( this );
		if (PCBreakpoint == cpu.PC) {
			shouldRun = false;
		}
		shouldStep = false;
	}
}

void Gameboy::Reset() {
	ResetMemory();
	apu.reset();
	cpu.Reset(skipBios);
	ppu.Reset();
}

void Gameboy::LoadCart(const char * path) {
	if ( cart != nullptr ) {
		delete cart;
	}
	cart = Cartridge::LoadFromFile(path);
	Reset();
}

void Gameboy::DebugDraw() {
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
	ImGui::Text("0x%02x", cpu.BC.high.Get()); ImGui::NextColumn();
	ImGui::Text("0x%02x", cpu.BC.low.Get()); ImGui::NextColumn();
	ImGui::Columns(2, "registers 16bits");
	ImGui::Separator();
	ImGui::Text("0x%04x", (uint16)(cpu.A.Get() << 8) | (cpu.F.Get())); ImGui::NextColumn();
	ImGui::Text("0x%04x", cpu.BC.Get()); ImGui::NextColumn();
	ImGui::Columns(4, "registers");
	ImGui::Separator();
	ImGui::Text("D"); ImGui::NextColumn();
	ImGui::Text("E"); ImGui::NextColumn();
	ImGui::Text("H"); ImGui::NextColumn();
	ImGui::Text("L"); ImGui::NextColumn();
	ImGui::Separator();
	ImGui::Text("0x%02x", cpu.DE.high.Get()); ImGui::NextColumn();
	ImGui::Text("0x%02x", cpu.DE.low.Get()); ImGui::NextColumn();
	ImGui::Text("0x%02x", cpu.HL.high.Get()); ImGui::NextColumn();
	ImGui::Text("0x%02x", cpu.HL.low.Get()); ImGui::NextColumn();
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
	ImGui::Text("Next instruction: %s", Cpu::s_instructionsNames[Read(cpu.PC)]);
	ImGui::Checkbox( "Skip bios", &skipBios );

	static char buf[64] = "";
	ImGui::InputText("Break at PC: ", buf, 64, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);
	if (buf[0] != '\0') {
		PCBreakpoint = strtol(buf, nullptr, 16);
	}
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
		ppu.DebugDraw( this );
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
			byte romValue = Read(addr);

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
				byte arg = Read(addr + 1);
				if (colored) {
					ImGui::TextColored(ImVec4(1, 1, 0, 1), "0x%04x %s %#x", addr, instructionName, arg);
				} else {
					ImGui::Text("0x%04x %s %#x", addr, instructionName, arg);
				}
				addr++;
			}
			if (instructionSize == 3) {
				byte val1 = Read(addr + 1);
				byte val2 = Read(addr + 2);
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
		if (cart != nullptr) {
			mem_edit.DrawWindow("ROM", cart->GetRawMemory(), cart->GetRawMemorySize(), 0x0);
		}
	}
}