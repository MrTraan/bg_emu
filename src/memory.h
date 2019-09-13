#pragma once

#include "gb_emu.h"
#include "rom.h"

#define TIMA 0xff05
#define TMA 0xff06
#define TAC 0xff07
#define DIV 0xff04

struct Memory {
	Memory(Cartridge * _cart);
	void Reset();

	Cartridge * cart;
	byte highRAM[0x100];
	byte VRAM[0x4000];
	byte VRAMBankIndex;

	// Work Ram bank 0-7
	byte WorkRam[0x9000];
	byte WorkRamBankIndex;

	byte OAM[0xa0];

	byte inputMask;

	bool hdmaActive = false;
	byte hdmaLength = 0;

	void Write(uint16 addr, byte value);
	byte Read(uint16 addr);
	void WriteHighRam(uint16 addr, byte value);
	byte ReadHighRam(uint16 addr);



	void HDMATransfer();
	void DMATransfer(byte value);
	void DMATransfer_GBC() { DEBUG_BREAK; }
};
