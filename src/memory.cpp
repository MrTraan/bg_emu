#include <string.h>
#include "gb_emu.h"
#include "memory.h"

static byte basicROM[0x8000];
static byte basicROMRam[0x8000];
static byte BIOS[0x100] = {
	0x31, 0xFE, 0xFF, 0xAF, 0x21, 0xFF, 0x9F, 0x32, 0xCB, 0x7C, 0x20, 0xFB, 0x21, 0x26, 0xFF, 0x0E,
	0x11, 0x3E, 0x80, 0x32, 0xE2, 0x0C, 0x3E, 0xF3, 0xE2, 0x32, 0x3E, 0x77, 0x77, 0x3E, 0xFC, 0xE0,
	0x47, 0x11, 0x04, 0x01, 0x21, 0x10, 0x80, 0x1A, 0xCD, 0x95, 0x00, 0xCD, 0x96, 0x00, 0x13, 0x7B,
	0xFE, 0x34, 0x20, 0xF3, 0x11, 0xD8, 0x00, 0x06, 0x08, 0x1A, 0x13, 0x22, 0x23, 0x05, 0x20, 0xF9,
	0x3E, 0x19, 0xEA, 0x10, 0x99, 0x21, 0x2F, 0x99, 0x0E, 0x0C, 0x3D, 0x28, 0x08, 0x32, 0x0D, 0x20,
	0xF9, 0x2E, 0x0F, 0x18, 0xF3, 0x67, 0x3E, 0x64, 0x57, 0xE0, 0x42, 0x3E, 0x91, 0xE0, 0x40, 0x04,
	0x1E, 0x02, 0x0E, 0x0C, 0xF0, 0x44, 0xFE, 0x90, 0x20, 0xFA, 0x0D, 0x20, 0xF7, 0x1D, 0x20, 0xF2,
	0x0E, 0x13, 0x24, 0x7C, 0x1E, 0x83, 0xFE, 0x62, 0x28, 0x06, 0x1E, 0xC1, 0xFE, 0x64, 0x20, 0x06,
	0x7B, 0xE2, 0x0C, 0x3E, 0x87, 0xF2, 0xF0, 0x42, 0x90, 0xE0, 0x42, 0x15, 0x20, 0xD2, 0x05, 0x20,
	0x4F, 0x16, 0x20, 0x18, 0xCB, 0x4F, 0x06, 0x04, 0xC5, 0xCB, 0x11, 0x17, 0xC1, 0xCB, 0x11, 0x17,
	0x05, 0x20, 0xF5, 0x22, 0x23, 0x22, 0x23, 0xC9, 0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
	0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
	0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
	0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E, 0x3c, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x4C,
	0x21, 0x04, 0x01, 0x11, 0xA8, 0x00, 0x1A, 0x13, 0xBE, 0x20, 0xFE, 0x23, 0x7D, 0xFE, 0x34, 0x20,
	0xF5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xFB, 0x86, 0x20, 0xFE, 0x3E, 0x01, 0xE0, 0x50 };

Memory::Memory(Cartridge * _cart) : cart(_cart) {
	memset(VRAM, 0, 0x4000);
	memset(WorkRam, 0, 0x9000);
	memset(OAM, 0, 0xa0);
	memset(highRAM, 0, 0x100);

	WorkRamBankIndex = 1;
	VRAMBankIndex = 0;

	memcpy(basicROM, BIOS, 0x100);

	highRAM[0x0f] = 0;

}

void Memory::Write(uint16 addr, byte value) {
	if (addr < 0x100) {
		return;
	}
	else if (addr < 0x8000) {
		cart->Write(addr, value);
	}
	else if (addr < 0xA000) {
		// VRAM banking
		uint16 bankOffset = VRAMBankIndex * 0x2000;
		VRAM[addr - 0x8000 + bankOffset] = value;
	}
	else if (addr < 0xC000) {
		basicROMRam[addr - 0xA000] = value;
	}
	else if (addr < 0xD000) {
		// Work RAM, bank 0
		WorkRam[addr - 0xC000] = value;
	}
	else if (addr < 0xE000) {
		// Work RAM with banking
		WorkRam[addr - 0xC000 + (WorkRamBankIndex * 0x1000)] = value;
	}
	else if (addr < 0xFE00) {
		// Echo RAM, don't know yet what to do with that
		//DEBUG_BREAK;
	}
	else if (addr < 0xFEA0) {
		// Object Attribute Memory
		OAM[addr - 0xFE00] = value;
	}
	else if (addr < 0xFF00) {
		// Unusable memory
		//DEBUG_BREAK;
	}
	else {
		WriteHighRam(addr, value);
	}
}

byte Memory::Read(uint16 addr) {
	if (highRAM[0x50] == 0 && addr < 0x100) {
		return BIOS[addr];
	}
	switch ((addr & 0xf000) >> 12) { // Switch on 4th byte
	case 0x0:
	case 0x1:
	case 0x2:
	case 0x3:
	case 0x4:
	case 0x5:
	case 0x6:
	case 0x7:
		return cart->Read(addr);

	case 0x8:
	case 0x9: {
		// VRAM banking
		uint16 bankOffset = VRAMBankIndex * 0x2000;
		return VRAM[addr - 0x8000 + bankOffset];
	}

	case 0xa:
	case 0xb:
		// ROM RAM
		return basicROMRam[addr - 0xa000];

	case 0xc:
	case 0xd:
		// Work RAM, bank 0
		return WorkRam[addr - 0xc000];
	case 0xe:
		// Work RAM with banking
		return WorkRam[addr - 0xc000 + (WorkRamBankIndex * 0x1000)];
	case 0xf: {
		if (addr < 0xFE00) {
			// ECHO RAM
			return WorkRam[addr - 0xf000];
		}
		else if (addr < 0xFEA0) {
			// Object Attribute Memory
			return OAM[addr - 0xFE00];
		}
		else if (addr < 0xFF00) {
			// Unusable memory
			return 0;
		}
		else {
			return ReadHighRam(addr);
		}
		
	}
	}
	return 0xFF;
}

byte Memory::ReadHighRam(uint16 addr) {
	if (addr == 0xff00) {
//		DEBUG_BREAK; // JOYPAD
	}
	else if (addr >= 0xff10 && addr <= 0xff26) {
//		DEBUG_BREAK; // READ SOUND
	}
	else if (addr >= 0xff30 && addr <= 0xff3f) {
//		DEBUG_BREAK; // READ SOUND WAVE FORM
	}
	else if (addr == 0xff0f) {
		return highRAM[0x0f] | 0xe0;
	}
	else if (addr > 0xff72 && addr <= 0xff77) {
		// Unkown
		DEBUG_BREAK;
	}
	else if (addr == 0xff68) {
		// BG palette index, only for GBC
		DEBUG_BREAK;
	}
	else if (addr == 0xff69) {
		// BG palette data, only for GBC
		DEBUG_BREAK;
	}
	else if (addr == 0xff6a) {
		// Sprite palette index, only for GBC
		DEBUG_BREAK;
	}
	else if (addr == 0xff6b) {
		// Sprite palette data, only for GBC
		DEBUG_BREAK;
	}
	else if (addr == 0xff4d) {
		// speed switch
		DEBUG_BREAK;
	}
	else if (addr == 0xff4f) {
		return VRAMBankIndex;
	}
	else if (addr == 0xff70) {
		return WorkRamBankIndex;
	}
	else {
		return highRAM[addr - 0xff00];
	}
}

void Memory::WriteHighRam(uint16 addr, byte value) {
	if (addr >= 0xfa0 && addr < 0xfeff) {
		// You can't write here, sorry!
		return;
	}
	if (addr >= 0xff10 && addr < 0xff26) {
		//DEBUG_BREAK; // Writing to sound
		return;
	}
	if (addr >= 0xff30 && addr <= 0xff3f) {
		//DEBUG_BREAK; // Writing to sound waveform
		return;
	}

	byte lowPart = BIT_LOW_8(addr);

	switch (lowPart) {
	case 0x02:
		//DEBUG_BREAK; // Serial transfer control 
		break;
	case 0x04:
		DEBUG_BREAK; // divider register
		break;
	case 0x05:
	case 0x06:
		// TIMA and TMA (timers)
		highRAM[lowPart] = value;
		break;
	case 0x07:
		// Starting or stopping the timer
		//DEBUG_BREAK;
		break;
	case 0x41:
		highRAM[0x41] = value | 0x80;
		break;
	case 0x44:
		// Scanline register
		highRAM[0x44] = value;
		break;
	case 0x46:
		DMATransfer(value);
		break;
	case 0x4d:
	case 0x4f:
	case 0x55:
	case 0x68:
	case 0x69:
	case 0x6a:
	case 0x6b:
	case 0x70:
		// CGB stuff
		DEBUG_BREAK;
		break;
	default:
		highRAM[lowPart] = value;
	}
}

void Memory::DMATransfer(byte value) {
	uint16 addr = (uint16)value << 8;
	for (uint16 i = 0; i < 0xa0; i++) {
		Write(0xfe00 + i, Read(addr + i));
	}
}

void Memory::HDMATransfer() {
	if (!hdmaActive) {
		return;
	}

	uint16 source = (uint16)highRAM[0x51] << 8 | (uint16)highRAM[0x52] & 0xfff0;
	uint16 dest = (uint16)highRAM[0x53] << 8 | (uint16)highRAM[0x54] & 0x1ff0;
	dest += 0x8000;

	for (uint16 i = 0; i < 0x10; i++) {
		Write(dest, Read(source));
		dest++;
		source++;
	}


	highRAM[0x51] = BIT_HIGH_8(source);
	highRAM[0x52] = BIT_LOW_8(source);
	highRAM[0x53] = BIT_HIGH_8(dest);
	highRAM[0x54] = dest & 0xf0;

	if (hdmaLength > 0) {
		hdmaLength--;
		highRAM[0x55] = hdmaLength;
	} else {
		// HDMA has finished
		highRAM[0x55] = 0xff;
		hdmaActive = false;
	}
}