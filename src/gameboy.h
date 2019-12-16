#pragma once
#include "cpu.h"
#include "memory.h"
#include "rom.h"
#include "ppu.h"
#include "sound/Gb_Apu.h"

#define TIMA 0xff05
#define TMA	 0xff06
#define TAC	 0xff07
#define DIV	 0xff04

struct Memory {
	byte highRAM[ 0x100 ];
	byte VRAM[ 0x4000 ];
	byte workRAM[ 0x9000 ];
	byte OAM[ 0x100 ];

	// Work Ram bank 0-7
	byte workRAMBankIndex;
	byte VRAMBankIndex;

	byte inputMask;

	byte hdmaLength = 0;
	bool hdmaActive = false;
};

struct Gameboy {
	Cpu		cpu;
	Memory	mem;
	Ppu		ppu;
	Gb_Apu	apu;

	Cartridge * cart = nullptr;

	bool	shouldRun = true;
	bool	shouldStep = false;
	int		PCBreakpoint = -1;
	bool	skipBios = false;

	void RunOneFrame();

	void Reset();
	void LoadCart( const char * path );

	void SerializeSaveState( const char * path );
	void LoadSaveState( const char * path );

	void DebugDraw();

	void ResetMemory();
	// Memory
	void Write( uint16 addr, byte value );
	byte Read( uint16 addr );
	void WriteHighRam( uint16 addr, byte value );
	byte ReadHighRam( uint16 addr );
	void HDMATransfer();
	void DMATransfer( byte value );
	void DMATransfer_GBC() { DEBUG_BREAK; }
	void RaiseInterupt( byte code );
};