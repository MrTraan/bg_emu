#include <stdlib.h>
#include <algorithm>
#include <stdio.h>
#include "gameboy.h"
#include <imgui/imgui.h>
#include <imgui/imgui_memory_editor.h>

static MemoryEditor mem_edit;

void Gameboy::RunOneFrame() {
	cpu.cpuTime = 0;
	constexpr int maxClocksThisFrame = GBEMU_CLOCK_SPEED / 60;

	while ( cpu.cpuTime < maxClocksThisFrame * cpu.speed && ( shouldRun || shouldStep ) ) {
		int clocks = 4;
		if ( !cpu.isOnHalt ) {
			clocks = cpu.ExecuteNextOPCode( this );
			totalInstructions++;
		}
		cpu.cpuTime += clocks;
		ppu.Update( clocks, this );
		cpu.UpdateTimer( clocks, this );
		cpu.cpuTime += cpu.ProcessInterupts( this );
		if ( PCBreakpoint == cpu.PC || instructionCountBreakpoint == totalInstructions ) {
			shouldRun = false;
		}
		shouldStep = false;
	}
}

void Gameboy::Reset() {
	if ( cart == nullptr ) {
		return;
	}
	totalInstructions = 0;
	ResetMemory();
	if ( cart->mode == DMG || (cart->mode == CGB_DMG && Cartridge::forceDMGMode )) {
		cpu.Reset( skipBios, false);
	} else {
		cpu.Reset( skipBios, true);
	}
	ppu.Reset();

	apu.reset();
	soundBuffer.clear();
	sound.stop();
	gbemu_assert( sound.start( sample_rate, 2 ) == nullptr );
}

void Gameboy::LoadCart( const char * path ) {
	if ( cart != nullptr ) {
		delete cart;
	}
	cart = Cartridge::LoadFromFile( path );
	Reset();
}

void Gameboy::SerializeSaveState( const char * path ) {
	FILE * fh = fopen( path, "wb" );
	if ( fh == nullptr ) {
		printf( "Could not find save state file %s\n", path );
		DEBUG_BREAK;
		return;
	}
	fwrite( &cpu, sizeof( Cpu ), 1, fh );
	fwrite( &mem, sizeof( Memory ), 1, fh );
	fwrite( &ppu.scanlineCounter, sizeof( int ), 1, fh );

	fclose( fh );
}

void Gameboy::LoadSaveState( const char * path ) {
	FILE * fh = fopen( path, "rb" );
	if ( fh == nullptr ) {
		printf( "Could not find save state file %s\n", path );
		DEBUG_BREAK;
		return;
	}

	fread( &cpu, sizeof( Cpu ), 1, fh );
	fread( &mem, sizeof( Memory ), 1, fh );
	fread( &ppu.scanlineCounter, sizeof( int ), 1, fh );

	fclose( fh );
}

void Gameboy::DebugDraw() {
	static float volume = 50.0f;
	if ( ImGui::SliderFloat( "Volume", &volume, 0.0f, 100.0f ) ) {
		if ( volume > 100.0f ) {
			volume = 100.0f;
		}
		apu.volume( volume / 100 );
	}
	ImGui::Text( "Cpu speed: %d\n", cpu.speed );
	ImGui::Columns( 4, "registers" );
	ImGui::Separator();
	ImGui::Text( "A" );
	ImGui::NextColumn();
	ImGui::Text( "F" );
	ImGui::NextColumn();
	ImGui::Text( "B" );
	ImGui::NextColumn();
	ImGui::Text( "C" );
	ImGui::NextColumn();
	ImGui::Separator();
	ImGui::Text( "0x%02x", cpu.A.Get() );
	ImGui::NextColumn();
	ImGui::Text( "0x%02x", cpu.F.Get() );
	ImGui::NextColumn();
	ImGui::Text( "0x%02x", cpu.BC.high.Get() );
	ImGui::NextColumn();
	ImGui::Text( "0x%02x", cpu.BC.low.Get() );
	ImGui::NextColumn();
	ImGui::Columns( 2, "registers 16bits" );
	ImGui::Separator();
	ImGui::Text( "0x%04x", ( uint16 )( cpu.A.Get() << 8 ) | ( cpu.F.Get() ) );
	ImGui::NextColumn();
	ImGui::Text( "0x%04x", cpu.BC.Get() );
	ImGui::NextColumn();
	ImGui::Columns( 4, "registers" );
	ImGui::Separator();
	ImGui::Text( "D" );
	ImGui::NextColumn();
	ImGui::Text( "E" );
	ImGui::NextColumn();
	ImGui::Text( "H" );
	ImGui::NextColumn();
	ImGui::Text( "L" );
	ImGui::NextColumn();
	ImGui::Separator();
	ImGui::Text( "0x%02x", cpu.DE.high.Get() );
	ImGui::NextColumn();
	ImGui::Text( "0x%02x", cpu.DE.low.Get() );
	ImGui::NextColumn();
	ImGui::Text( "0x%02x", cpu.HL.high.Get() );
	ImGui::NextColumn();
	ImGui::Text( "0x%02x", cpu.HL.low.Get() );
	ImGui::NextColumn();
	ImGui::Columns( 2, "registers 16bits" );
	ImGui::Separator();
	ImGui::Text( "0x%04x", cpu.DE.Get() );
	ImGui::NextColumn();
	ImGui::Text( "0x%04x", cpu.HL.Get() );
	ImGui::NextColumn();
	ImGui::Columns( 2, "PC and SP" );
	ImGui::Separator();
	ImGui::Text( "PC" );
	ImGui::NextColumn();
	ImGui::Text( "SP" );
	ImGui::NextColumn();
	ImGui::Separator();
	ImGui::Text( "0x%04x", cpu.PC );
	ImGui::NextColumn();
	ImGui::Text( "0x%04x", cpu.SP.Get() );
	ImGui::NextColumn();
	ImGui::Columns( 4, "flags" );
	ImGui::Separator();
	ImGui::Text( "Z" );
	ImGui::NextColumn();
	ImGui::Text( "N" );
	ImGui::NextColumn();
	ImGui::Text( "H" );
	ImGui::NextColumn();
	ImGui::Text( "C" );
	ImGui::NextColumn();
	ImGui::Separator();
	ImGui::Text( "%d", cpu.GetZ() );
	ImGui::NextColumn();
	ImGui::Text( "%d", cpu.GetN() );
	ImGui::NextColumn();
	ImGui::Text( "%d", cpu.GetH() );
	ImGui::NextColumn();
	ImGui::Text( "%d", cpu.GetC() );
	ImGui::NextColumn();

	ImGui::Columns( 1 );
	ImGui::Separator();
	ImGui::Text( "Last instruction: %s", Cpu::s_instructionsNames[ cpu.lastInstructionOpCode ] );
	ImGui::Text( "Next instruction: %s", Cpu::s_instructionsNames[ Read( cpu.PC ) ] );
	ImGui::Checkbox( "Skip bios", &skipBios );
	ImGui::SameLine();
	ImGui::Checkbox( "Print OPCodes", &dumpOPcodesToStdout );

	ImGui::Text("Total instructions: %lld", totalInstructions);

	static char PCBreakpointStr[ 64 ] = "";
	ImGui::InputText( "Break at PC: ", PCBreakpointStr, 64, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase );
	if ( PCBreakpointStr[ 0 ] != '\0' ) {
		PCBreakpoint = strtol( PCBreakpointStr, nullptr, 16 );
	}
	static char InstructionBreakpointStr[ 64 ] = "";
	ImGui::InputText( "Break at instruction Count: ", InstructionBreakpointStr, 64, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase );
	if ( InstructionBreakpointStr[ 0 ] != '\0' ) {
		instructionCountBreakpoint = strtoull( InstructionBreakpointStr, nullptr, 10 );
	}

	if ( ImGui::Button( "Step" ) ) {
		shouldStep = true;
	}

	static int showRomCode = false;
	if ( ImGui::Button( showRomCode ? "Hide ROM Code" : "Show ROM Code" ) ) {
		showRomCode = !showRomCode;
	}
	ImGui::SameLine();
	if ( ImGui::Button( "Generate ROM Code" ) && cart != nullptr ) {
		cart->GenerateSourceCode();
	}

	static int showMemoryInspector = false;
	if ( ImGui::TreeNode( "Memory" ) ) {
		ImGui::TreePop();
		ImGui::Text( "workRAM bank: %d", mem.workRAMBankIndex );
		ImGui::Text( "VRAM bank: %d", mem.VRAMBankIndex );
		ImGui::Text( "Input mask: %d", mem.inputMask );
		ImGui::Text( "hdma length: %d", mem.hdmaLength );
		ImGui::Text( "hdma active: %d", mem.hdmaActive );
		if ( ImGui::Button( showMemoryInspector ? "Hide Memory" : "Show Memory" ) ) {
			showMemoryInspector = !showMemoryInspector;
		}
	}

	if ( ImGui::TreeNode( "Pixel Processing Unit" ) ) {
		ppu.DebugDraw( this );
		ImGui::TreePop();
	}
	if ( cart != nullptr ) {
		if ( ImGui::TreeNode( "Cartridge" ) ) {
			cart->DebugDraw();
			ImGui::TreePop();
		}
	}

	if ( showRomCode && cart != nullptr ) {
		ImGui::Begin( "ROM Code" );
		ImGui::BeginGroup();

		ImGui::BeginChild( ImGui::GetID( "ROM CODE DUMP" ) );
		int remainingLines = 100; // TMP: it is really slow to display a huge list with ImGUI, this is temporary to reduce pressure
		bool pcFound = false;
		int PC = cart->DebugResolvePC(cpu.PC);
		size_t startIndex = 0;

		//binary search our address
		auto it = std::lower_bound(cart->sourceCodeAddresses.begin(), cart->sourceCodeAddresses.end(), PC);
		if (it == cart->sourceCodeAddresses.end() || *it != PC) {
			startIndex = 0;
		} else {
			std::size_t index = std::distance(cart->sourceCodeAddresses.begin(), it);
			startIndex = MAX((int64)index - 100, 0);
		}   

		for ( size_t i = startIndex; i < startIndex + 200 && i < cart->sourceCodeLines.size(); i++) {
			const std::string & line = cart->sourceCodeLines[i];
			bool colored = false;
			if ( cart->sourceCodeAddresses[i] == PC ) {
				colored = true;
				pcFound = true;
				ImGui::SetScrollHereY( 0.5f ); // 0.0f:top, 0.5f:center, 1.0f:bottom
			}
			if ( colored ) {
				ImGui::TextColored( ImVec4( 1, 1, 0, 1 ), line.c_str() );
			} else {
				ImGui::Text( line.c_str() );
			}
			if (pcFound && remainingLines-- <= 0) {
				break;
			}
		}
		ImGui::EndChild();
		ImGui::EndGroup();
		ImGui::End();
	}

	if ( showMemoryInspector ) {
		mem_edit.DrawWindow( "VRAM", mem.VRAM, 0x4000, 0x0 );
		mem_edit.DrawWindow( "HighRAM", mem.highRAM, 0x100, 0x0 );
		mem_edit.DrawWindow( "OAM", mem.OAM, 0xa0, 0x0 );
		mem_edit.DrawWindow( "WorkRAM", mem.workRAM, 0x9000, 0x0 );
		if ( cart != nullptr ) {
			mem_edit.DrawWindow( "ROM", cart->GetRawMemory(), cart->rawMemorySize, 0x0 );
		}
	}
}