#pragma once

#include "gb_emu.h"

struct Memory {
	// Reference to cartridge?

	byte highRAM[0x100];
	byte VRAM[0x4000];
	byte VRAMBankIndex;

	// Work Ram bank 0-7
	byte WorkRam[0x9000];
	byte WorkRamBankIndex;

	byte OAM[0x100];

	// Initial values of memory
	Memory();

	void Write(uint16 addr, byte value);
	byte Read(uint16 addr);
};