#pragma once

#include "gb_emu.h"

// There are 5 different types of ROM, for now we handle only the basic type (Tetris is one of them)

class Cartridge {
public:
	virtual byte Read(uint16 addr) = 0;
	virtual void Write(uint16 addr, byte val) = 0;
	virtual void WriteRAM(uint16 addr, byte val) = 0;
	virtual byte * GetRawMemory() = 0;
	virtual int GetRawMemorySize() = 0;

	static Cartridge * LoadFromFile(const char * path);
};

class ROM : public Cartridge {
public:
	byte data[0x8000];

	virtual byte Read(uint16 addr) override { return data[addr]; }
	virtual void Write(uint16 addr, byte val) override { }
	virtual void WriteRAM(uint16 addr, byte val) override { }

	virtual byte * GetRawMemory() { return data; }
	virtual int GetRawMemorySize() { return 0x8000; }
};

class MBC1 : public Cartridge {
public:
	byte data[0x80000];
	uint16 romBank = 1;
	bool romBanking = false;

	byte ram[0x8000];
	uint16 ramBank = 1;
	bool ramEnabled = false;
	
	virtual byte Read(uint16 addr) override;

	virtual void Write(uint16 addr, byte val) override;
	
	virtual void WriteRAM(uint16 addr, byte val) override;

	virtual byte * GetRawMemory() { return data; }
	virtual int GetRawMemorySize() { return 0x10000; }
};

class MBC5 : public Cartridge {
public:
	byte data[0x100000];
	uint16 romBank = 1;
	bool romBanking = false;

	byte ram[0x20000];
	uint16 ramBank = 1;
	bool ramEnabled = false;
	
	virtual byte Read(uint16 addr) override;

	virtual void Write(uint16 addr, byte val) override;
	
	virtual void WriteRAM(uint16 addr, byte val) override;

	virtual byte * GetRawMemory() { return data; }
	virtual int GetRawMemorySize() { return 0x10000; }
};