#pragma once

#include "gb_emu.h"

struct Register {
	void Set(uint16 val) {
		word = val;
		ApplyMask();
	}
	uint16 Get() { return word; }

	uint8 GetHigh() {
		return (uint8)(word >> 8);
	}
	uint8 GetLow() {
		return (uint8)(word & 0xFF);
	}
	void SetHigh(uint8 val) {
		word = (word & 0xFF) + ((uint16)val << 8);
		ApplyMask();
		
	}
	void SetLow(uint8 val) {
		word = (word & 0xFF00) + val;
		ApplyMask();
	}

	void ApplyMask() {
		word &= mask;
	}
	
	uint16 mask = 0xFFFF; // Mask of the possible value in this register. Only used by AF whose lower byte can't be set
private:
	uint16 word = 0;
};

struct Cpu {
	Register AF;
	Register BC;
	Register DE;
	Register HL;

	uint16 PC;
	Register SP;

	// int divider

	// Initialize CPU with default values
	Cpu() {
		PC = 0x100;
		AF.Set(0x01B0);
		BC.Set(0);
		DE.Set(0xFF56);
		HL.Set(0x000D);
		SP.Set(0xFFFE);

		AF.mask = 0xFFF0;
	}

	void SetFlag(uint8 index, bool val) {
		if (val == true) {
			AF.SetLow(BIT_SET(AF.GetLow(), index));
		} else {
			AF.SetLow(BIT_UNSET(AF.GetLow(), index));
		}
	}

	void SetZ(bool val) { SetFlag(7, val); }
	void SetN(bool val) { SetFlag(6, val); }
	void SetH(bool val) { SetFlag(5, val); }
	void SetC(bool val) { SetFlag(4, val); }

	bool GetZ() { return (AF.Get() >> 7 & 1) == 1; }
	bool GetN() { return (AF.Get() >> 6 & 1) == 1; }
	bool GetH() { return (AF.Get() >> 5 & 1) == 1; }
	bool GetC() { return (AF.Get() >> 4 & 1) == 1; }
};
