#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "rom.h"
#include <imgui/imgui.h>

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
		gbemu_assert( sizeof( rom->data ) >= size );
		memset( rom->data, 0, sizeof( rom->data ) );
		memcpy( rom->data, cartData, size );
		cart = rom;
	} else if ( ROMIsMBC1( cartType ) ) {
		MBC1 * rom = new MBC1();
		gbemu_assert( sizeof( rom->data ) >= size );
		memset( rom->data, 0, sizeof( rom->data ) );
		memcpy( rom->data, cartData, size );
		cart = rom;
	} else if ( ROMIsMBC5( cartType ) ) {
		MBC5 * rom = new MBC5();
		gbemu_assert( sizeof( rom->data ) >= size );
		memset( rom->data, 0, sizeof( rom->data ) );
		memcpy( rom->data, cartData, size );
		cart = rom;
	} else {
		DEBUG_BREAK;
	}

	if ( cart != nullptr ) {
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
	}
	delete[] cartData;
	return cart;
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

byte MBC5::Read( uint16 addr ) {
	if ( addr < 0x4000 ) {
		return data[ addr ];
	} else if ( addr < 0x8000 ) {
		return data[ addr - 0x4000 + romBank * 0x4000 ];
	} else {
		return ram[ ramBank * 0x2000 + addr - 0xa000 ];
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
