#include "cpu.h"
#include "memory.h"

static int opcodeCyclesCost[] = {
	//  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
	1, 3, 2, 2, 1, 1, 2, 1, 5, 2, 2, 2, 1, 1, 2, 1, // 0
	0, 3, 2, 2, 1, 1, 2, 1, 3, 2, 2, 2, 1, 1, 2, 1, // 1
	2, 3, 2, 2, 1, 1, 2, 1, 2, 2, 2, 2, 1, 1, 2, 1, // 2
	2, 3, 2, 2, 3, 3, 3, 1, 2, 2, 2, 2, 1, 1, 2, 1, // 3
	1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, // 4
	1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, // 5
	1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, // 6
	2, 2, 2, 2, 2, 2, 0, 2, 1, 1, 1, 1, 1, 1, 2, 1, // 7
	1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, // 8
	1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, // 9
	1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, // a
	1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, // b
	2, 3, 3, 4, 3, 4, 2, 4, 2, 4, 3, 0, 3, 6, 2, 4, // c
	2, 3, 3, 0, 3, 4, 2, 4, 2, 4, 3, 0, 3, 0, 2, 4, // d
	3, 3, 2, 0, 0, 4, 2, 4, 4, 1, 4, 0, 0, 0, 2, 4, // e
	3, 3, 2, 1, 0, 4, 2, 4, 3, 2, 4, 1, 0, 0, 2, 4, // f
};

void Cpu::Add( Register8 & reg, byte val, bool useCarry ) {
	byte  valReg = reg.Get();
	byte  carry  = GetC() && useCarry ? 1 : 0;
	int16 total  = valReg + val + carry;
	reg.Set( (byte)total );

	SetZ( total == 0 );
	SetN( false );
	SetH( ( val & 0xF ) + ( valReg & 0xF ) + carry > 0xF );
	SetC( total > 0xFF );
}

void Cpu::Sub( Register8 & reg, byte val, bool useCarry ) {
	byte  valReg = reg.Get();
	byte  carry  = GetC() && useCarry ? 1 : 0;
	int16 total  = valReg - val - carry;
	reg.Set( (byte)total );

	SetZ( total == 0 );
	SetN( true );
	SetH( ( int16 )( val & 0xF ) - ( valReg & 0xF ) - carry < 0 );
	SetC( total < 0 );
}

void Cpu::And( Register8 & reg, byte val ) {
	byte valReg = reg.Get();
	byte total  = valReg & val;
	reg.Set( (byte)total );

	SetZ( total == 0 );
	SetN( false );
	SetH( true );
	SetC( false );
}

void Cpu::Or( Register8 & reg, byte val ) {
	byte valReg = reg.Get();
	byte total  = valReg | val;
	reg.Set( (byte)total );

	SetZ( total == 0 );
	SetN( false );
	SetH( false );
	SetC( false );
}

void Cpu::Xor( Register8 & reg, byte val ) {
	byte valReg = reg.Get();
	byte total  = valReg ^ val;
	reg.Set( (byte)total );

	SetZ( total == 0 );
	SetN( false );
	SetH( false );
	SetC( false );
}

void Cpu::Cp( Register8 & reg, byte val ) {
	byte valReg = reg.Get();
	byte total  = valReg - val;
	SetZ( total == 0 );
	SetN( true );
	SetH( ( valReg & 0x0f ) > ( val & 0x0f ) );
	SetC( valReg > val );
}

void Cpu::Inc( Register8 & reg ) {
	byte valReg = reg.Get();
	byte total  = valReg + 1;
	SetZ( total == 0 );
	SetN( false );
	SetH( ( valReg & 0x0f ) + 1 > 0x0f );
}

void Cpu::Dec( Register8 & reg ) {
	byte valReg = reg.Get();
	byte total  = valReg - 1;
	SetZ( total == 0 );
	SetN( false );
	SetH( ( valReg & 0x0f ) > 0x0f );
}

void Cpu::Add16( Register16 & reg, uint16 val ) {
	uint16 valReg = reg.Get();
	int	   total  = valReg + val;
	reg.Set( (uint16)total );
	SetN( false );
	SetH( ( valReg & 0xfff ) > ( total & 0xfff ) );
	SetC( total > 0xffff );
}

void Cpu::Add16Signed( Register16 & reg, byte val ) {
	uint16 valReg = reg.Get();
	int	   total  = valReg + val;
	reg.Set( (uint16)total );
	SetZ( false );
	SetN( false );
	SetH( ( ( valReg ^ val ^ total ) & 0x10 ) == 0x10 );
	SetC( ( ( valReg ^ val ^ total ) & 0x100 ) == 0x100 );
}

void Cpu::Inc16( Register16 & reg ) {
	reg.Set( reg.Get() + 1 );
}

void Cpu::Dec16( Register16 & reg ) {
	reg.Set( reg.Get() - 1 );
}

byte Cpu::PopPC( Memory * mem ) {
	byte opcode = mem->Read( PC );
	PC++;
	return opcode;
}

uint16 Cpu::PopPC16( Memory * mem ) {
	byte val1 = mem->Read( PC );
	PC++;
	byte val2 = mem->Read( PC );
	PC++;
	return ( (uint16)val2 << 8 ) | val1;
}

void Halt() {
} // TODO: don't know what to do yet with that

// Return Cycles used
int Cpu::ExecuteNextOPCode( Memory * mem ) {
	constexpr bool HIGH		 = true;
	constexpr bool LOW		 = false;
	byte		   opcode	 = PopPC( mem );
	int			   ticksUsed = opcodeCyclesCost[ opcode ] * 4;

	// All instructions are detailled here : https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html

	switch ( opcode ) {
		case 0x0: {
			// NOP
			break;
		}

		case 0x01: {
			// LD BC, d16
			uint16 val = PopPC16( mem );
			BC.Set( val );
			break;
		}

		case 0x02: {
			// LD (BC), A
			uint16 addr = BC.Get();
			byte   val  = A.Get();
			mem->Write( addr, val );
			break;
		}

		case 0x03: {
			// INC BC
			Inc16( BC );
			break;
		}

		case 0x04: {
			// INC B
			Inc( B );
			break;
		}

		case 0x05: {
			// DEC B
			Dec( B );
			break;
		}

		case 0x06: {
			// LD B, d8
			byte val = PopPC( mem );
			B.Set( val );
			break;
		}

		case 0x07: {
			// RLCA
			byte val = A.Get();
			byte res = ( val << 1 ) || ( val >> 7 ); // Put last bit first
			A.Set( res );
			SetZ( false );
			SetN( false );
			SetH( false );
			SetC( val > 0x7f );
			break;
		}

		case 0x08: {
			// LD (a16), SP
			uint16 addr = PopPC16( mem );
			mem->Write( addr, SP.low.Get() );
			mem->Write( addr + 1, SP.high.Get() );
			break;
		}

		case 0x09: {
			// ADD HL, BC
			Add16( HL, BC.Get() );
			break;
		}

		case 0x0a: {
			// LD A, (BC)
			byte val = mem->Read( BC.Get() );
			A.Set( val );
			break;
		}

		case 0x0b: {
			// DEC BC
			Dec16( BC );
			break;
		}

		case 0x0c: {
			// INC C
			Inc( C );
			break;
		}

		case 0x0d: {
			// DEC C
			Dec( C );
			break;
		}

		case 0x0e: {
			// LD C, d8
			byte val = PopPC( mem );
			C.Set( val );
			break;
		}

		case 0x0f: {
			// RRCA
			byte val = A.Get();
			byte res = ( val >> 1 ) | ( ( val & 1 ) << 7 );
			A.Set( res );
			SetZ( false );
			SetN( false );
			SetH( false );
			SetC( res > 0x7f );
			break;
		}

		case 0x10: {
			// STOP 0
			Halt();
			PopPC( mem ); // Read next byte, because this instruction is actualy 2 bytes : 0x10 0x00, no idea why
			break;
		}

		case 0x11: {
			// LD DE, d16
			BC.Set( PopPC16( mem ) );
			break;
		}

		case 0x12: {
			// LD (DE), A
			mem->Write( DE.Get(), A.Get() );
			break;
		}

		case 0x13: {
			// INC DE
			Inc16( DE );
			break;
		}

		case 0x14: {
			// INC D
			Inc( D );
			break;
		}

		case 0x15: {
			// DEC D
			Dec( D );
			break;
		}

		case 0x16: {
			// LD D, d8
			D.Set( PopPC( mem ) );
			break;
		}

		case 0x17: {
			// RLA
			byte val   = A.Get();
			byte carry = GetC() ? 1 : 0;
			byte res   = ( val << 1 ) + carry;
			A.Set( res );
			SetZ( false );
			SetN( false );
			SetH( false );
			SetC( res > 0x7f );
			break;
		}

		case 0x18: {
			// JR r8
			uint16 addr = PC + PopPC( mem );
			PC			= addr;
			break;
		}

		case 0x19: {
			// ADD HL, DE
			Add16( HL, DE.Get() );
			break;
		}

		case 0x1a: {
			// LD A, (DE)
			A.Set( mem->Read( DE.Get() ) );
			break;
		}

		case 0x1b: {
			// DEC DE
			Dec16( DE );
			break;
		}

		case 0x1c: {
			// INC E
			Inc( E );
			break;
		}

		case 0x1d: {
			// DEC E
			Dec( E );
			break;
		}

		case 0x1e: {
			// LD E, d8
			E.Set( PopPC( mem ) );
			break;
		}

		case 0x1f: {
			// RRA
			byte val   = A.Get();
			byte carry = GetC() ? 0x80 : 0;
			byte res   = ( val >> 1 ) | carry;
			A.Set( res );
			SetZ( false );
			SetN( false );
			SetH( false );
			SetC( ( 1 & val ) == 1 );
			break;
		}

		case 0x20: {
			// JR NZ, r8
			if ( !GetZ() ) {
				uint16 addr = PC + PopPC( mem );
				PC			= addr;
				ticksUsed += 4;
			}
			break;
		}

		case 0x21: {
			// LD HL, d16
			HL.Set( PopPC16( mem ) );
			break;
		}

		case 0x22: {
			// LD (HL+), A
			mem->Write( HL.Get(), A.Get() );
			Inc16( HL );
			break;
		}

		case 0x23: {
			// INC HL
			Inc16( HL );
			break;
		}

		case 0x24: {
			// INC H
			Inc( H );
			break;
		}

		case 0x25: {
			// DEC H
			Dec( H );
			break;
		}

		case 0x26: {
			// LD H, d8
			H.Set( PopPC( mem ) );
			break;
		}

		case 0x27: {
			// DAA
			// When this instruction is executed, the A register is BCD
			// corrected using the contents of the flags. The exact process
			// is the following: if the least significant four bits of A
			// contain a non-BCD digit (i. e. it is greater than 9) or the
			// H flag is set, then 0x60 is added to the register. Then the
			// four most significant bits are checked. If this more significant
			// digit also happens to be greater than 9 or the C flag is set,
			// then 0x60 is added.
			if ( !GetN() ) {
				if ( GetC() || A.Get() > 0x99 ) {
					A.Set( A.Get() + 0x60 );
					SetC( true );
				}
				if ( GetH() || ( A.Get() & 0xF ) > 0x9 ) {
					A.Set( A.Get() + 0x06 );
					SetH( false );
				}
			} else if ( GetC() && GetH() ) {
				A.Set( A.Get() + 0x9a );
				SetH( false );
			} else if ( GetC() ) {
				A.Set( A.Get() + 0xa0 );
			} else if ( GetH() ) {
				A.Set( A.Get() + 0xfa );
				SetH( false );
			}
			SetZ( A.Get() == 0 );
			break;
		}

		case 0x28: {
			// JR Z, r8
			if ( GetZ() ) {
				uint16 addr = PC + PopPC( mem );
				PC			= addr;
				ticksUsed += 4;
			}
			break;
		}

		case 0x29: {
			// ADD HL, HL
			Add16( HL, HL.Get() );
			break;
		}

		case 0x2a: {
			// LD A, (HL+)
			A.Set( mem->Read( HL.Get() ) );
			Inc16( HL );
			break;
		}

		case 0x2b: {
			// DEC HL
			Dec16( HL );
			break;
		}

		case 0x2c: {
			// INC L
			Inc( L );
			break;
		}

		case 0x2d: {
			// DEC L
			Dec( L );
			break;
		}

		case 0x2e: {
			// LD L, d8
			L.Set( PopPC( mem ) );
			break;
		}

		case 0x2f: {
			// CPL
			A.Set( 0xFF ^ A.Get() );
			SetN( true );
			SetH( true );
			break;
		}

		case 0x30: {
			// JR NC, r8
			if ( !GetC() ) {
				uint16 addr = PC + PopPC( mem );
				PC			= addr;
				ticksUsed += 4;
			}
			break;
		}

		case 0x31: {
			// LD SP, d16
			SP.Set( PopPC16( mem ) );
			break;
		}

		case 0x32: {
			// LD (HL-), A
			mem->Write( HL.Get(), A.Get() );
			Dec16( HL );
			break;
		}

		case 0x33: {
			// INC SP
			Inc16( SP );
			break;
		}

		case 0x34: {
			// INC (HL)
			// We are using A as a temporary register for the value pointed to by HL
			byte previousA = A.Get();
			A.Set( mem->Read( HL.Get() ) );
			Inc( A );
			mem->Write( HL.Get(), A.Get() );
			A.Set( previousA );
			break;
		}

		case 0x35: {
			// DEC (HL)
			// We are using A as a temporary register for the value pointed to by HL
			byte previousA = A.Get();
			A.Set( mem->Read( HL.Get() ) );
			Dec( A );
			mem->Write( HL.Get(), A.Get() );
			A.Set( previousA );
			break;
		}

		case 0x36: {
			// LD (HL), d8
			mem->Write( HL.Get(), PopPC( mem ) );
			break;
		}

		case 0x37: {
			// SCF
			SetN( false );
			SetH( false );
			SetC( true );
			break;
		}

		case 0x38: {
			// JR C, r8
			if ( GetC() ) {
				uint16 addr = PC + PopPC( mem );
				PC			= addr;
				ticksUsed += 4;
			}
			break;
		}

		case 0x39: {
			// ADD HL, SP
			Add16( HL, SP.Get() );
			break;
		}

		case 0x3a: {
			// LD A, (HL-)
			A.Set( mem->Read( HL.Get() ) );
			Dec16( HL );
			break;
		}

		case 0x3b: {
			// DEC SP
			Dec16( SP );
			break;
		}

		case 0x3c: {
			// INC A
			Inc( A );
			break;
		}

		case 0x3d: {
			// DEC A
			Dec( A );
			break;
		}

		case 0x3e: {
			// LD A, d8
			A.Set( PopPC( mem ) );
			break;
		}

		case 0x3f: {
			// CCF
			SetN( false );
			SetH( false );
			SetC( !GetC() );
			break;
		}

		case 0x40: {
			// LD B, B
			B.Set( B.Get() );
			break;
		}

		case 0x41: {
			// LD B, C
			B.Set( C.Get() );
			break;
		}

		case 0x42: {
			// LD B, D
			B.Set( D.Get() );
			break;
		}

		case 0x43: {
			// LD B, E
			B.Set( E.Get() );
			break;
		}

		case 0x44: {
			// LD B, H
			B.Set( H.Get() );
			break;
		}

		case 0x45: {
			// LD B, L
			B.Set( L.Get() );
			break;
		}

		case 0x46: {
			// LD B, (HL)
			B.Set( mem->Read(HL.Get()) );
			break;
		}

		case 0x47: {
			// LD B, A
			B.Set( A.Get() );
			break;
		}

		case 0x48: {
			// LD C, B
			C.Set( B.Get() );
			break;
		}

		case 0x49: {
			// LD C, C
			C.Set( C.Get() );
			break;
		}

		case 0x4a: {
			// LD C, D
			C.Set( D.Get() );
			break;
		}

		case 0x4b: {
			// LD C, E
			C.Set( E.Get() );
			break;
		}

		case 0x4c: {
			// LD C, H
			C.Set( H.Get() );
			break;
		}

		case 0x4d: {
			// LD C, L
			C.Set( L.Get() );
			break;
		}

		case 0x4e: {
			// LD C, (HL)
			C.Set( mem->Read( HL.Get() ) );
			break;
		}

		case 0x4f: {
			// LD C, A
			C.Set( A.Get() );
			break;
		}

		case 0x50: {
			// LD D, B
			D.Set( B.Get() );
			break;
		}

		case 0x51: {
			// LD D, C
			D.Set( C.Get() );
			break;
		}

		case 0x52: {
			// LD D, D
			D.Set( D.Get() );
			break;
		}

		case 0x53: {
			// LD D, E
			D.Set( E.Get() );
			break;
		}

		case 0x54: {
			// LD D, H
			D.Set( H.Get() );
			break;
		}

		case 0x55: {
			// LD D, L
			D.Set( L.Get() );
			break;
		}

		case 0x56: {
			// LD D, (HL)
			D.Set( mem->Read( HL.Get() ) );
			break;
		}

		case 0x57: {
			// LD D, A
			D.Set( A.Get() );
			break;
		}

		case 0x58: {
			// LD E, B
			E.Set( B.Get() );
			break;
		}

		case 0x59: {
			// LD E, C
			E.Set( C.Get() );
			break;
		}

		case 0x5a: {
			// LD E, D
			E.Set( D.Get() );
			break;
		}

		case 0x5b: {
			// LD E, E
			E.Set( E.Get() );
			break;
		}

		case 0x5c: {
			// LD E, H
			E.Set( H.Get() );
			break;
		}

		case 0x5d: {
			// LD E, L
			E.Set( L.Get() );
			break;
		}

		case 0x5e: {
			// LD E, (HL)
			E.Set( mem->Read( HL.Get() ) );
			break;
		}

		case 0x5f: {
			// LD E, A
			E.Set( A.Get() );
			break;
		}

		case 0x60: {
			// LD H, B
			H.Set( B.Get() );
			break;
		}

		case 0x61: {
			// LD H, C
			H.Set( C.Get() );
			break;
		}

		case 0x62: {
			// LD H, D
			H.Set( D.Get() );
			break;
		}

		case 0x63: {
			// LD H, E
			H.Set( E.Get() );
			break;
		}

		case 0x64: {
			// LD H, H
			H.Set( H.Get() );
			break;
		}

		case 0x65: {
			// LD H, L
			H.Set( L.Get() );
			break;
		}

		case 0x66: {
			// LD H, (HL)
			H.Set( mem->Read( HL.Get() ) );
			break;
		}

		case 0x67: {
			// LD H, A
			H.Set( A.Get() );
			break;
		}

		case 0x68: {
			// LD L, B
			L.Set( B.Get() );
			break;
		}

		case 0x69: {
			// LD L, C
			L.Set( C.Get() );
			break;
		}

		case 0x6a: {
			// LD L, D
			L.Set( D.Get() );
			break;
		}

		case 0x6b: {
			// LD L, E
			L.Set( E.Get() );
			break;
		}

		case 0x6c: {
			// LD L, H
			L.Set( H.Get() );
			break;
		}

		case 0x6d: {
			// LD L, L
			L.Set( L.Get() );
			break;
		}

		case 0x6e: {
			// LD L, (HL)
			L.Set( mem->Read( HL.Get() ) );
			break;
		}

		case 0x6f: {
			// LD L, A
			L.Set( A.Get() );
			break;
		}

		case 0x70: {
			// LD (HL), B
			mem->Write( HL.Get(), B.Get() );
			break;
		}

		case 0x71: {
			// LD (HL), C
			mem->Write( HL.Get(), C.Get() );
			break;
		}

		case 0x72: {
			// LD (HL), D
			mem->Write( HL.Get(), D.Get() );
			break;
		}

		case 0x73: {
			// LD (HL), E
			mem->Write( HL.Get(), E.Get() );
			break;
		}

		case 0x74: {
			// LD (HL), H
			mem->Write( HL.Get(), H.Get() );
			break;
		}

		case 0x75: {
			// LD (HL), L
			mem->Write( HL.Get(), L.Get() );
			break;
		}

		case 0x76: {
			// HALT
			Halt();
			break;
		}

		case 0x77: {
			// LD (HL), A
			mem->Write( HL.Get(), A.Get() );
			break;
		}

		case 0x78: {
			// LD A, B
			A.Set( B.Get() );
			break;
		}

		case 0x79: {
			// LD A, C
			A.Set( C.Get() );
			break;
		}

		case 0x7a: {
			// LD A, D
			A.Set( D.Get() );
			break;
		}

		case 0x7b: {
			// LD A, E
			A.Set( E.Get() );
			break;
		}

		case 0x7c: {
			// LD A, H
			A.Set( H.Get() );
			break;
		}

		case 0x7d: {
			// LD A, L
			A.Set( L.Get() );
			break;
		}

		case 0x7e: {
			// LD A, (HL)
			A.Set( mem->Read( HL.Get() ) );
			break;
		}

		case 0x7f: {
			// LD A, A
			A.Set( A.Get() );
			break;
		}

		default: {
			// Unimplemented instruction
			DEBUG_BREAK;
		}
	}

	return ticksUsed;
}
