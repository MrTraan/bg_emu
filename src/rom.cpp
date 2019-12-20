#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "rom.h"
#include "cpu.h"
#include <imgui/imgui.h>

bool Cartridge::forceDMGMode = false;

Cartridge * Cartridge::LoadFromFile( const char * path ) {
	FILE * fh = fopen( path, "r" );
	if ( fh == nullptr ) {
		printf( "Could not find ROM file %s\n", path );
		DEBUG_BREAK;
		return nullptr;
	}
	fseek( fh, 0L, SEEK_END );
	long size = ftell( fh );
	rewind( fh );

	if ( size < 0x148 ) {
		// We won't be able to check cart type, this is not a valid rom
		fclose( fh );
		return nullptr;
	}

	byte * cartData = new byte[ size ];
	fread( cartData, size, 1, fh );
	fclose( fh );

	ROMType cartType = (ROMType)cartData[ 0x147 ];

	Cartridge * cart = nullptr;
	if ( ROMIsBasicROM( cartType ) ) {
		ROM * rom = new ROM();
		rom->data = cartData;
		cart = rom;
	} else if ( ROMIsMBC1( cartType ) ) {
		MBC1 * rom = new MBC1();
		cart = rom;
		rom->ramEnabled = ROMHasRAM( cartType );
	} else if ( ROMIsMBC3( cartType ) ) {
		MBC3 * rom = new MBC3();
		cart = rom;
		rom->ramEnabled = ROMHasRAM( cartType );
	} else if ( ROMIsMBC5( cartType ) ) {
		MBC5 * rom = new MBC5();
		cart = rom;
		rom->ramEnabled = ROMHasRAM( cartType );
	} else {
		DEBUG_BREAK;
	}

	if ( cart != nullptr ) {
		cart->data = cartData;
		cart->rawMemorySize = size;
		cart->type = cartType;
		if ( cartData[0x143] == 0x80 ) {
			cart->mode = CGB_DMG;
		} else if ( cartData[0x143] == 0xc0 ) {
			cart->mode = CGB_ONLY;
		} else {
			cart->mode = DMG;
		}

		strncpy( cart->romPath, path, 0x200 );
		// Game name is written between 0x134 and 0x142
		for ( int i = 0; i < 0xE; i++ ) {
			byte c = cart->Read( i + 0x134 );
			if ( c == ' ' ) {
				c = '_';
			}
			cart->romName[ i ] = tolower( c );
		}
		cart->romName[ 0xE ] = 0;
		//cart->GenerateSourceCode();
	}
	return cart;
}

void Cartridge::DebugDraw() {
	ImGui::Checkbox("Force DMG", &forceDMGMode);
	ImGui::Text("ROM size: %#llx", rawMemorySize);
}

void Cartridge::GenerateSourceCode() {
	char buf[100] = {};
	for ( int addr = 0; addr < rawMemorySize; addr++ ) {
		byte romValue = GetRawMemory()[addr];
		sourceCodeAddresses.push_back(addr);

		const char *	instructionName = Cpu::s_instructionsNames[ romValue ];
		byte			instructionSize = Cpu::s_instructionsSize[ romValue ];

		if ( instructionSize == 1 ) {
			snprintf(buf, 100, "0x%04x 0x%02x %s", addr, romValue, instructionName );
		}
		if ( instructionSize == 2 ) {
			byte arg = Read( addr + 1 );
			snprintf(buf, 100, "0x%04x 0x%02x %s %#x", addr, romValue, instructionName, arg );
			addr++;
		}
		if ( instructionSize == 3 ) {
			byte	val1 = Read( addr + 1 );
			byte	val2 = Read( addr + 2 );
			uint16	arg = ( (uint16)val2 << 8 ) | val1;
			snprintf(buf, 100, "0x%04x 0x%02x %s %#x", addr, romValue, instructionName, arg );
			addr += 2;
		}

		sourceCodeLines.push_back(buf);
	}
}

byte MBC1::Read( uint16 addr ) {
	if ( addr < 0x4000 ) {
		return data[ addr ];
	} else if ( addr < 0x8000 ) {
		return data[ addr - 0x4000 + romBank * 0x4000 ];
	} else {
		return ram[ ramBank * 0x2000 + addr - 0xa000 ];
	}
}

int MBC1::DebugResolvePC( uint16 PC ) {
	if ( PC < 0x4000 ) {
		return PC;
	} else if ( PC < 0x8000 ) {
		return PC - 0x4000 + romBank * 0x4000;
	} else {
		return 0;
	}
}

void MBC1::Write( uint16 addr, byte val ) {
	switch ( ( addr & 0xf000 ) >> 12 ) { // Switch on 4th byte
		case 0x0:
		case 0x1:
			if ( ( val & 0xf ) == 0xa ) {
				ramEnabled = true;
			} else if ( ( val & 0xf ) == 0x0 ) {
				ramEnabled = false;
			}
			break;

		case 0x2:
		case 0x3:
			// Sets rom bank number
			romBank = ( romBank & 0xe0 ) | ( val & 0x1f );
			if ( romBank == 0x00 || romBank == 0x20 || romBank == 0x40 || romBank == 0x60 ) { // Invalid values
				romBank++;
			}
			break;

		case 0x4:
		case 0x5:
			if ( romBanking ) {
				romBank = ( romBank & 0x1f ) | ( val & 0xe0 );
				if ( romBank == 0x00 || romBank == 0x20 || romBank == 0x40 || romBank == 0x60 ) { // Invalid values
					romBank++;
				}
			} else {
				ramBank = val & 0x3;
			}
			break;

		case 0x6:
		case 0x7:
			romBanking = !BIT_IS_SET( val, 1 );
			if ( romBanking ) {
				ramBank = 0;
			} else {
				romBank = romBank & 0x1f;
			}
			break;
	}
}

void MBC1::WriteRAM( uint16 addr, byte val ) {
	if ( ramEnabled ) {
		ram[ ramBank * 0x2000 + addr - 0xa000 ] = val;
	}
}

byte MBC3::Read( uint16 addr ) {
	if ( addr < 0x4000 ) {
		return data[ addr ];
	} else if ( addr < 0x8000 ) {
		return data[ addr - 0x4000 + romBank * 0x4000 ];
	} else {
		if ( ramBank >= 0x4 ) {
			if ( latched ) {
				return latchedRtc[ ramBank ];
			}
			return rtc[ ramBank ];
		}
		return ram[ ramBank * 0x2000 + addr - 0xa000 ];
	}
}

int MBC3::DebugResolvePC( uint16 PC ) {
	if ( PC < 0x4000 ) {
		return PC;
	} else if ( PC < 0x8000 ) {
		return PC - 0x4000 + romBank * 0x4000;
	} else {
		return 0;
	}
}

void MBC3::Write( uint16 addr, byte val ) {
	switch ( ( addr & 0xf000 ) >> 12 ) { // Switch on 4th byte
		case 0x0:
		case 0x1:
			ramEnabled = ( val & 0xa ) != 0;
			break;

		case 0x2:
		case 0x3:
			// Sets rom bank number
			romBank = val & 0x7f;
			if ( romBank == 0 ) {
				romBank = 1;
			}
			break;

		case 0x4:
		case 0x5:
			ramBank = val;
			break;

		case 0x6:
		case 0x7:
			if ( val == 0x1 ) {
				latched = false;
			} else if ( val == 0x0 ) {
				latched = true;
				memcpy( rtc, latchedRtc, sizeof(rtc) );
			}
	}
}

void MBC3::WriteRAM( uint16 addr, byte val ) {
	if ( ramEnabled ) {
		if ( ramBank >= 0x4 ) {
			rtc[ ramBank ] = val;
		} else {
			ram[ ramBank * 0x2000 + addr - 0xa000 ] = val;
		}
	}
}

byte MBC5::Read( uint16 addr ) {
	if ( addr < 0x4000 ) {
		return data[ addr ];
	} else if ( addr < 0x8000 ) {
		return data[ addr - 0x4000 + romBank * 0x4000 ];
	} else {
		return ram[ ramBank * 0x2000 + addr - 0xa000 ];
	}
}

int MBC5::DebugResolvePC( uint16 PC ) {
	if ( PC < 0x4000 ) {
		return PC;
	} else if ( PC < 0x8000 ) {
		return PC - 0x4000 + romBank * 0x4000;
	} else {
		return 0;
	}
}

void MBC5::Write( uint16 addr, byte val ) {
	switch ( ( addr & 0xf000 ) >> 12 ) { // Switch on 4th byte
		case 0x0:
		case 0x1:
			if ( ( val & 0xf ) == 0xa ) {
				ramEnabled = true;
			} else if ( ( val & 0xf ) == 0x0 ) {
				ramEnabled = false;
			}
			break;

		case 0x2:
			// Sets rom bank number
			romBank = ( romBank & 0x100 ) | val;
			break;

		case 0x3:
			romBank = ( romBank & 0xff ) | ( ( val & 0x01 ) << 8 );
			break;

		case 0x4:
		case 0x5:
			ramBank = val & 0xF;
			break;
	}
}

void MBC5::WriteRAM( uint16 addr, byte val ) {
	if ( ramEnabled ) {
		ram[ ramBank * 0x2000 + addr - 0xa000 ] = val;
	}
}

void MBC5::DebugDraw() {
	Cartridge::DebugDraw();
	ImGui::Text( "Rom bank: %d", romBank );
	ImGui::Text( "Ram bank: %d", ramBank );
}

bool ROMHasBattery( ROMType type ) {
	switch ( type ) {
		case CART_TYPE_MBC1_RAM_BATTERY:
		case CART_TYPE_MBC2_BATTERY:
		case CART_TYPE_ROM_RAM_BATTERY:
		case CART_TYPE_MMM01_RAM_BATTERY:
		case CART_TYPE_MBC3_TIMER_BATTERY:
		case CART_TYPE_MBC3_TIMER_RAM_BATTERY:
		case CART_TYPE_MBC3_RAM_BATTERY:
		case CART_TYPE_MBC4_RAM_BATTERY:
		case CART_TYPE_MBC5_RAM_BATTERY:
		case CART_TYPE_MBC5_RUMBLE_RAM_BATTERY:
		case CART_TYPE_HUC1_RAM_BATTERY:
			return true;
		default:
			return false;
	}
}

bool ROMHasRAM( ROMType type ) {
	switch ( type ) {
		case CART_TYPE_MBC1_RAM:
		case CART_TYPE_MBC1_RAM_BATTERY:
		case CART_TYPE_ROM_RAM:
		case CART_TYPE_ROM_RAM_BATTERY:
		case CART_TYPE_MMM01_RAM:
		case CART_TYPE_MMM01_RAM_BATTERY:
		case CART_TYPE_MBC3_TIMER_RAM_BATTERY:
		case CART_TYPE_MBC3_RAM:
		case CART_TYPE_MBC3_RAM_BATTERY:
		case CART_TYPE_MBC4_RAM:
		case CART_TYPE_MBC4_RAM_BATTERY:
		case CART_TYPE_MBC5_RAM:
		case CART_TYPE_MBC5_RAM_BATTERY:
		case CART_TYPE_MBC5_RUMBLE_RAM:
		case CART_TYPE_MBC5_RUMBLE_RAM_BATTERY:
		case CART_TYPE_HUC1_RAM_BATTERY:
			return true;
		default:
			return false;
	}
}

bool ROMIsBasicROM( ROMType type ) {
	switch ( type ) {
		case CART_TYPE_ROM:
		case CART_TYPE_ROM_RAM:
		case CART_TYPE_ROM_RAM_BATTERY:
			return true;
		default:
			return false;
	}
}

bool ROMIsMBC1( ROMType type ) {
	switch ( type ) {
		case CART_TYPE_MBC1:
		case CART_TYPE_MBC1_RAM:
		case CART_TYPE_MBC1_RAM_BATTERY:
			return true;
		default:
			return false;
	}
}

bool ROMIsMBC2( ROMType type ) {
	switch ( type ) {
		case CART_TYPE_MBC2:
		case CART_TYPE_MBC2_BATTERY:
			return true;
		default:
			return false;
	}
}

bool ROMIsMBC3( ROMType type ) {
	switch ( type ) {
		case CART_TYPE_MBC3_TIMER_BATTERY:
		case CART_TYPE_MBC3_TIMER_RAM_BATTERY:
		case CART_TYPE_MBC3:
		case CART_TYPE_MBC3_RAM:
		case CART_TYPE_MBC3_RAM_BATTERY:
			return true;
		default:
			return false;
	}
}

bool ROMIsMBC5( ROMType type ) {
	switch ( type ) {
		case CART_TYPE_MBC5:
		case CART_TYPE_MBC5_RAM:
		case CART_TYPE_MBC5_RAM_BATTERY:
		case CART_TYPE_MBC5_RUMBLE:
		case CART_TYPE_MBC5_RUMBLE_RAM:
		case CART_TYPE_MBC5_RUMBLE_RAM_BATTERY:
			return true;
		default:
			return false;
	}
}
