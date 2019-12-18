#pragma once

#include "gb_emu.h"
#include <vector>
#include <string>

// There are 5 different types of ROM, for now we handle only the basic type (Tetris is one of them)

enum ROMType : byte {
	CART_TYPE_ROM = 0x00,
	CART_TYPE_MBC1 = 0x01,
	CART_TYPE_MBC1_RAM = 0x02,
	CART_TYPE_MBC1_RAM_BATTERY = 0x03,
	CART_TYPE_MBC2 = 0x05,
	CART_TYPE_MBC2_BATTERY = 0x06,
	CART_TYPE_ROM_RAM = 0x08,
	CART_TYPE_ROM_RAM_BATTERY = 0x09,
	CART_TYPE_MMM01 = 0x0B,
	CART_TYPE_MMM01_RAM = 0x0C,
	CART_TYPE_MMM01_RAM_BATTERY = 0x0D,
	CART_TYPE_MBC3_TIMER_BATTERY = 0x0F,
	CART_TYPE_MBC3_TIMER_RAM_BATTERY = 0x10,
	CART_TYPE_MBC3 = 0x11,
	CART_TYPE_MBC3_RAM = 0x12,
	CART_TYPE_MBC3_RAM_BATTERY = 0x13,
	CART_TYPE_MBC4 = 0x15,
	CART_TYPE_MBC4_RAM = 0x16,
	CART_TYPE_MBC4_RAM_BATTERY = 0x17,
	CART_TYPE_MBC5 = 0x19,
	CART_TYPE_MBC5_RAM = 0x1A,
	CART_TYPE_MBC5_RAM_BATTERY = 0x1B,
	CART_TYPE_MBC5_RUMBLE = 0x1C,
	CART_TYPE_MBC5_RUMBLE_RAM = 0x1D,
	CART_TYPE_MBC5_RUMBLE_RAM_BATTERY = 0x1E,
	CART_TYPE_POCKET_CAMERA = 0xFC,
	CART_TYPE_BANDAI_TAMA5 = 0xFD,
	CART_TYPE_HUC3 = 0xFE,
	CART_TYPE_HUC1_RAM_BATTERY = 0xFF,
};

enum ColorMode : byte {
	DMG,
	CGB_DMG,
	CGB_ONLY,
};

bool ROMHasBattery( ROMType type );
bool ROMHasRAM( ROMType type );
bool ROMIsBasicROM( ROMType type );
bool ROMIsMBC1( ROMType type );
bool ROMIsMBC2( ROMType type );
bool ROMIsMBC3( ROMType type );
bool ROMIsMBC5( ROMType type );

class Cartridge {
public:
	virtual byte	Read( uint16 addr ) = 0;
	virtual void	Write( uint16 addr, byte val ) = 0;
	virtual void	WriteRAM( uint16 addr, byte val ) = 0;
	virtual byte *	GetRawMemory() = 0;
	virtual int		GetRawMemorySize() = 0;
	virtual int		DebugResolvePC(uint16 PC) = 0;

	static bool forceDMGMode;
	virtual void DebugDraw();

	static Cartridge * LoadFromFile( const char * path );

	ROMType type;
	ColorMode mode;
	char romName[ 0xF ];
	char romPath[ 0x200 ];
	int rawMemorySize = 0;

	std::vector<std::string> sourceCodeLines;
	std::vector<uint32> sourceCodeAddresses;

	void GenerateSourceCode();
};

class ROM : public Cartridge {
public:
	byte data[ 0x8000 ];

	virtual byte Read( uint16 addr ) override { return data[ addr ]; }
	virtual int DebugResolvePC(uint16 PC) override { return PC; }
	virtual void Write( uint16 addr, byte val ) override {}
	virtual void WriteRAM( uint16 addr, byte val ) override {}

	virtual byte *	GetRawMemory() { return data; }
	virtual int		GetRawMemorySize() { return 0x8000; }
};

class MBC1 : public Cartridge {
public:
	byte	data[ 0x80000 ];
	uint16	romBank = 1;
	bool	romBanking = false;

	byte	ram[ 0x8000 ];
	uint16	ramBank = 1;
	bool	ramEnabled = true;

	virtual byte Read( uint16 addr ) override;
	virtual void Write( uint16 addr, byte val ) override;
	virtual void WriteRAM( uint16 addr, byte val ) override;
	virtual int DebugResolvePC(uint16 PC) override;

	virtual byte *	GetRawMemory() { return data; }
	virtual int		GetRawMemorySize() { return 0x80000; }
};

class MBC5 : public Cartridge {
public:
	byte	data[ 0x100000 ];
	uint16	romBank = 1;
	bool	romBanking = false;

	byte	ram[ 0x20000 ];
	uint16	ramBank = 0;
	bool	ramEnabled = true;

	virtual byte Read( uint16 addr ) override;
	virtual void Write( uint16 addr, byte val ) override;
	virtual void WriteRAM( uint16 addr, byte val ) override;
	virtual int DebugResolvePC(uint16 PC) override;

	virtual byte *	GetRawMemory() { return data; }
	virtual int		GetRawMemorySize() { return 0x100000; }

	virtual void DebugDraw() override;
};