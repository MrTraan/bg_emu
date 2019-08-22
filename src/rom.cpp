#include <stdio.h>
#include <string.h>
#include "rom.h"

Cartridge * Cartridge::LoadFromFile(const char * path) {
	FILE * fh = fopen(path, "r");
	if (fh == nullptr) {
		printf("Could not find ROM file %s\n", path);
		DEBUG_BREAK;
		return nullptr;
	}
	fseek(fh, 0L, SEEK_END);
	long size = ftell(fh);
	rewind(fh);

	byte * cartData = new byte[size];
	fread(cartData, size, 1, fh);
	fclose(fh);

	byte cartType = cartData[0x147];

	Cartridge * cart = nullptr;
	if (cartType == 0) {
		ROM * rom = new ROM();
		memcpy(rom->data, cartData, sizeof(rom->data));
		cart = rom;
	} else if (cartType == 0x01) {
		MBC1 * rom = new MBC1();
		memcpy(rom->data, cartData, sizeof(rom->data));
		cart = rom;
	} else {
		DEBUG_BREAK;
	}

	delete [] cartData;
	return cart;
}

byte MBC1::Read(uint16 addr) {
	if (addr < 0x4000) {
		return data[addr];
	} else if (addr < 0x8000) {
		return data[addr - 0x4000 + romBank * 0x4000];
	} else {
		return ram[ramBank * 0x2000 + addr - 0xa000];
	}
}

void MBC1::Write(uint16 addr, byte val) {
	switch ((addr & 0xf000) >> 12) { // Switch on 4th byte
	case 0x0:
	case 0x1:
		if ((val & 0xf) == 0xa) {
			ramEnabled = true;
		} else if ((val & 0xf) == 0x0) {
			ramEnabled = false;
		}
		break;

	case 0x2:
	case 0x3:
		// Sets rom bank number
		romBank = (romBank & 0xe0) | (val & 0x1f);
		if (romBank == 0x00 || romBank == 0x20 || romBank == 0x40 || romBank == 0x60) { // Invalid values
			romBank++;
		}
		break;

	case 0x4:
	case 0x5:
		if (romBanking) {
			romBank = (romBank & 0x1f) | (val & 0xe0);
			if (romBank == 0x00 || romBank == 0x20 || romBank == 0x40 || romBank == 0x60) { // Invalid values
				romBank++;
			}
		} else {
			ramBank = val & 0x3;
		}

	case 0x6:
	case 0x7:
		romBanking = !BIT_IS_SET(val, 1);
		if (romBanking) {
			ramBank = 0;
		} else {
			romBank = romBank & 0x1f;
		}
	}
}

void MBC1::WriteRAM(uint16 addr, byte val) {
	if (ramEnabled) {
		ram[ramBank * 0x2000 + addr - 0xa000] = val;
	}
}
