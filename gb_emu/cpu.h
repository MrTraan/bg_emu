#pragma once

#include "gb_emu.h"

struct Memory;

struct Register8 {
	byte mask = 0xFF;

	byte Get() { return value; }
	void Set( uint8 newVal ) {
		value = newVal;
		ApplyMask();
	}

	void ApplyMask() { value &= mask; }

private:
	byte value;
};

struct Register16 {
	Register8 high;
	Register8 low;

	void Set( uint16 val ) {
		high.Set( val >> 8 );
		low.Set( val & 0xFF );
	}

	uint16 Get() { return ( (uint16)high.Get() << 8 ) + low.Get(); }
};

struct Cpu {
	Register16  AF;
	Register16  BC;
	Register16  DE;
	Register16  HL;
	Register8 & A = AF.high;
	Register8 & F = AF.low;
	Register8 & B = BC.high;
	Register8 & C = BC.low;
	Register8 & D = DE.high;
	Register8 & E = DE.low;
	Register8 & H = HL.high;
	Register8 & L = HL.low;

	uint16	   PC;
	Register16 SP;

	// int divider

	// Initialize CPU with default values
	Cpu() {
		PC = 0x100;
		AF.Set( 0x01B0 );
		BC.Set( 0 );
		DE.Set( 0xFF56 );
		HL.Set( 0x000D );
		SP.Set( 0xFFFE );

		F.mask = 0xF0;
	}

	int	   ExecuteNextOPCode( Memory * mem );
	byte   PopPC( Memory * mem );
	uint16 PopPC16( Memory * mem );

	void Add( Register8 & reg, byte val, bool useCarry );
	void Sub( Register8 & reg, byte val, bool useCarry );
	void And( Register8 & reg, byte val );
	void Or( Register8 & reg, byte val );
	void Xor( Register8 & reg, byte val );
	void Cp( Register8 & reg, byte val );
	void Inc( Register8 & reg );
	void Dec( Register8 & reg );

	void Add16( Register16 & reg, uint16 val );
	void Add16Signed( Register16 & reg, byte val );
	void Inc16( Register16 & reg );
	void Dec16( Register16 & reg );

	void SetFlag( uint8 index, bool val ) {
		if ( val == true ) {
			A.Set( BIT_SET( F.Get(), index ) );
		} else {
			A.Set( BIT_UNSET( F.Get(), index ) );
		}
	}

	void SetZ( bool val ) { SetFlag( 7, val ); }
	void SetN( bool val ) { SetFlag( 6, val ); }
	void SetH( bool val ) { SetFlag( 5, val ); }
	void SetC( bool val ) { SetFlag( 4, val ); }

	bool GetZ() { return BIT_IS_SET( AF.Get(), 7 ); }
	bool GetN() { return BIT_IS_SET( AF.Get(), 6 ); }
	bool GetH() { return BIT_IS_SET( AF.Get(), 5 ); }
	bool GetC() { return BIT_IS_SET( AF.Get(), 4 ); }
};
