#include "gb_emu.h"
#include "memory.h"

Memory::Memory() {
	highRAM[ 0x04 ] = 0x1E;
	highRAM[ 0x05 ] = 0x00;
	highRAM[ 0x06 ] = 0x00;
	highRAM[ 0x07 ] = 0xF8;
	highRAM[ 0x0F ] = 0xE1;
	highRAM[ 0x10 ] = 0x80;
	highRAM[ 0x11 ] = 0xBF;
	highRAM[ 0x12 ] = 0xF3;
	highRAM[ 0x14 ] = 0xBF;
	highRAM[ 0x16 ] = 0x3F;
	highRAM[ 0x17 ] = 0x00;
	highRAM[ 0x19 ] = 0xBF;
	highRAM[ 0x1A ] = 0x7F;
	highRAM[ 0x1B ] = 0xFF;
	highRAM[ 0x1C ] = 0x9F;
	highRAM[ 0x1E ] = 0xBF;
	highRAM[ 0x20 ] = 0xFF;
	highRAM[ 0x21 ] = 0x00;
	highRAM[ 0x22 ] = 0x00;
	highRAM[ 0x23 ] = 0xBF;
	highRAM[ 0x24 ] = 0x77;
	highRAM[ 0x25 ] = 0xF3;
	highRAM[ 0x26 ] = 0xF1;
	highRAM[ 0x40 ] = 0x91;
	highRAM[ 0x41 ] = 0x85;
	highRAM[ 0x42 ] = 0x00;
	highRAM[ 0x43 ] = 0x00;
	highRAM[ 0x45 ] = 0x00;
	highRAM[ 0x47 ] = 0xFC;
	highRAM[ 0x48 ] = 0xFF;
	highRAM[ 0x49 ] = 0xFF;
	highRAM[ 0x4A ] = 0x00;
	highRAM[ 0x4B ] = 0x00;
	highRAM[ 0xFF ] = 0x00;

	WorkRamBankIndex = 1;
}

void Memory::Write(uint16 addr, byte value) {
	if (addr < 0x8000) {
		// Writing to the ROM
		DEBUG_BREAK;
	} else if (addr < 0xA000) {
		// VRAM banking
		uint16 bankOffset = VRAMBankIndex * 0x2000;
		VRAM[addr - 0x8000 + bankOffset] = value;
	} else if (addr < 0xC000) {
		// Writing to the cartridge RAM
		DEBUG_BREAK;
	} else if (addr < 0xD000) {
		// Work RAM, bank 0
		WorkRam[addr - 0xC000] = value;
	} else if (addr < 0xE000) {
		// Work RAM with banking
		WorkRam[addr - 0xC000 + (WorkRamBankIndex * 0x1000)] = value;
	} else if (addr < 0xFE00) {
		// Echo RAM, don't know yet what to do with that
		DEBUG_BREAK;
	} else if (addr < 0xFEA0) {
		// Object Attribute Memory
		OAM[addr - 0xFE00] = value;
	} else if (addr < 0xFF00) {
		// Unusable memory
		DEBUG_BREAK;
	} else {
		// high ram?
		DEBUG_BREAK;
	}
}

byte Memory::Read(uint16 addr) {
	if (addr < 0x8000) {
		// Reading the ROM
		DEBUG_BREAK;
	} else if (addr < 0xA000) {
		// VRAM banking
		uint16 bankOffset = VRAMBankIndex * 0x2000;
		return VRAM[addr - 0x8000 + bankOffset];
	} else if (addr < 0xC000) {
		// Reading cartridge RAM
		DEBUG_BREAK;
	} else if (addr < 0xD000) {
		// Work RAM, bank 0
		return WorkRam[addr - 0xC000];
	} else if (addr < 0xE000) {
		// Work RAM with banking
		return WorkRam[addr - 0xC000 + (WorkRamBankIndex * 0x1000)];
	} else if (addr < 0xFE00) {
		// Echo RAM, don't know yet what to do with that
		DEBUG_BREAK;
	} else if (addr < 0xFEA0) {
		// Object Attribute Memory
		return OAM[addr - 0xFE00];
	} else if (addr < 0xFF00) {
		// Unusable memory
		DEBUG_BREAK;
	} else {
		// high ram?
		DEBUG_BREAK;
	}
	return 0xFF;
}