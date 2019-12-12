#include "cpu.h"
#include "memory.h"

#define INVALID_OP DEBUG_BREAK

const char * Cpu::s_instructionsNames[ 0x100 ] = {
	"NOP",		   "LD BC, d16", "LD (BC), A",  "INC BC",	  "INC B",		 "DEC B",	   "LD B, d8",	  "RLCA",		"LD (a16), SP", "ADD HL, BC",
	"LD A, (BC)",  "DEC BC",	 "INC C",		"DEC C",	  "LD C, d8",	 "RRCA",	   "STOP 0",	  "LD DE, d16", "LD (DE), A",   "INC DE",
	"INC D",	   "DEC D",		 "LD D, d8",	"RLA",		  "JR r8",		 "ADD HL, DE", "LD A, (DE)",  "DEC DE",		"INC E",		"DEC E",
	"LD E, d8",	   "RRA",		 "JR NZ, r8",   "LD HL, d16", "LD (HL+), A", "INC HL",	   "INC H",		  "DEC H",		"LD H, d8",		"DAA",
	"JR Z, r8",	   "ADD HL, HL", "LD A, (HL+)", "DEC HL",	  "INC L",		 "DEC L",	   "LD L, d8",	  "CPL",		"JR NC, r8",	"LD SP, d16",
	"LD (HL-), A", "INC SP",	 "INC (HL)",	"DEC (HL)",   "LD (HL), d8", "SCF",		   "JR C, r8",	  "ADD HL, SP", "LD A, (HL-)",  "DEC SP",
	"INC A",	   "DEC A",		 "LD A, d8",	"CCF",		  "LD B, B",	 "LD B, C",	   "LD B, D",	  "LD B, E",	"LD B, H",		"LD B, L",
	"LD B, (HL)",  "LD B, A",	 "LD C, B",		"LD C, C",	  "LD C, D",	 "LD C, E",	   "LD C, H",	  "LD C, L",	"LD C, (HL)",   "LD C, A",
	"LD D, B",	   "LD D, C",	 "LD D, D",		"LD D, E",	  "LD D, H",	 "LD D, L",	   "LD D, (HL)",  "LD D, A",	"LD E, B",		"LD E, C",
	"LD E, D",	   "LD E, E",	 "LD E, H",		"LD E, L",	  "LD E, (HL)",  "LD E, A",	   "LD H, B",	  "LD H, C",	"LD H, D",		"LD H, E",
	"LD H, H",	   "LD H, L",	 "LD H, (HL)",  "LD H, A",	  "LD L, B",	 "LD L, C",	   "LD L, D",	  "LD L, E",	"LD L, H",		"LD L, L",
	"LD L, (HL)",  "LD L, A",	 "LD (HL), B",  "LD (HL), C", "LD (HL), D",  "LD (HL), E", "LD (HL), H",  "LD (HL), L", "HALT",			"LD (HL), A",
	"LD A, B",	   "LD A, C",	 "LD A, D",		"LD A, E",	  "LD A, H",	 "LD A, L",	   "LD A, (HL)",  "LD A, A",	"ADD A,B",		"ADD A,C",
	"ADD A,D",	   "ADD A,E",	 "ADD A,H",		"ADD A,L",	  "ADD A,(HL)",  "ADD A,A",	   "ADC A,B",	  "ADC A,C",	"ADC A,D",		"ADC A,E",
	"ADC A,H",	   "ADC A,L",	 "ADC A,(HL)",  "ADC A,A",	  "SUB A, B",	 "SUB A, C",   "SUB A, D",	  "SUB A, E",   "SUB A, H",		"SUB A, L",
	"SUB A, (HL)", "SUB A, A",   "SBC A,B",		"SBC A,C",	  "SBC A,D",	 "SBC A,E",	   "SBC A,H",	  "SBC A,L",	"SBC A,(HL)",   "SBC A,A",
	"AND A, B",	   "AND A, C",   "AND A, D",	"AND A, E",   "AND A, H",	 "AND A, L",   "AND A, (HL)", "AND A, A",   "XOR A, B",		"XOR A, C",
	"XOR A, D",	   "XOR A, E",   "XOR A, H",	"XOR A, L",   "XOR A, (HL)", "XOR A, A",   "OR B",		  "OR C",		"OR D",			"OR E",
	"OR H",		   "OR L",		 "OR (HL)",		"OR A",		  "CP B",		 "CP C",	   "CP D",		  "CP E",		"CP H",			"CP L",
	"CP (HL)",	   "CP A",		 "RET NZ",		"POP BC",	  "JP NZ, a16",  "JP a16",	   "CALL NZ",	  "PUSH BC",	"ADD A, d8",	"RST 0x00",
	"RET Z",	   "RET",		 "JP Z, a16",   "PREFIX CB",  "CALL Z",		 "CALL a16",   "ADC A, d8",   "RST 0x08",   "RET NC",		"POP DE",
	"JP NC, a16",  "INVALID_OP", "CALL NC",		"PUSH DE",	  "SUB A, d8",   "RST 0x10",   "RET C",		  "RETI",		"JP C, a16",	"INVALID_OP",
	"CALL C",	   "INVALID_OP", "SBC A, d8",   "RST 0x18",   "LDH (a8), A", "POP HL",	   "LD (C), A",   "INVALID_OP", "INVALID_OP",   "PUSH HL",
	"AND A, d8",   "RST 0x20",   "ADD SP, r8",  "JP HL",	  "LD (a16), A", "INVALID_OP", "INVALID_OP",  "INVALID_OP", "XOR A, d8",	"RST 0x28",
	"LDH A, (a8)", "POP AF",	 "LD A, (C)",   "DI",		  "INVALID_OP",  "PUSH AF",	   "OR A, d8",	  "RST 0x30",   "LD HL, SP+r8", "LD SP, HL",
	"LD A, (a16)", "EI",		 "INVALID_OP",  "INVALID_OP", "CP A, d8",	 "RST 0x38",
};

byte Cpu::s_instructionsSize[ 0x100 ] = { 1, 3, 1, 1, 1, 1, 2, 1, 3, 1, 1, 1, 1, 1, 2, 1, 2, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1, 2, 3, 1, 1, 1,
										  1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1, 2, 3, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
										  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
										  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
										  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
										  1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 3, 1, 2, 1, 1, 1, 3, 1, 3, 3, 2, 1, 1, 1, 3, 1, 3, 1, 2, 1, 1, 1, 3, 1, 3, 1,
										  2, 1, 2, 1, 2, 1, 1, 1, 2, 1, 2, 1, 3, 1, 1, 1, 2, 1, 2, 1, 2, 1, 1, 1, 2, 1, 2, 1, 3, 1, 1, 1, 2, 1 };

void Cpu::ExecuteInstruction( byte opcode ) {
	switch ( opcode ) {
		case 0x00: {
			break;
			// NOP
			break;
		}
		case 0x01: {
			// LD BC, d16
			uint16 val = PopPC16();
			BC.Set( val );
			break;
		}
		case 0x02: {
			// LD (BC), A
			uint16	addr = BC.Get();
			byte	val = A.Get();
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
			byte val = PopPC();
			B.Set( val );
			break;
		}
		case 0x07: {
			// RLCA
			byte val = A.Get();
			byte res = ( val << 1 ) | ( val >> 7 ); // Put last bit first
			A.Set( res );
			SetZ( false );
			SetN( false );
			SetH( false );
			SetC( val > 0x7f );
			break;
		}
		case 0x08: {
			// LD (a16), SP
			uint16 addr = PopPC16();
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
			byte val = PopPC();
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
			// Halt();
			PopPC(); // Read next byte, because this instruction is actualy 2 bytes : 0x10 0x00, no idea why
					 // TODO: Game boy color change speed
			break;
		}
		case 0x11: {
			// LD DE, d16
			DE.Set( PopPC16() );
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
			D.Set( PopPC() );
			break;
		}
		case 0x17: {
			// RLA
			byte val = A.Get();
			byte carry = GetC() ? 1 : 0;
			byte res = ( val << 1 ) + carry;
			A.Set( res );
			SetZ( false );
			SetN( false );
			SetH( false );
			SetC( val > 0x7f );
			break;
		}
		case 0x18: {
			// JR r8
			int addr = (int8)PopPC() + (int)PC;
			PC = (uint16)addr;
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
			E.Set( PopPC() );
			break;
		}
		case 0x1f: {
			// RRA
			byte val = A.Get();
			byte carry = GetC() ? 0x80 : 0;
			byte res = ( val >> 1 ) | carry;
			A.Set( res );
			SetZ( false );
			SetN( false );
			SetH( false );
			SetC( ( 1 & val ) == 1 );
			break;
		}
		case 0x20: {
			// JR NZ, r8
			int8 jump = (int8)PopPC();
			if ( !GetZ() ) {
				uint16 addr = PC + jump;
				PC = addr;
				additionnalTicks += 4;
			}
			break;
		}
		case 0x21: {
			// LD HL, d16
			HL.Set( PopPC16() );
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
			H.Set( PopPC() );
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
			int8 jump = (int8)PopPC();
			if ( GetZ() ) {
				uint16 addr = PC + jump;
				PC = addr;
				additionnalTicks += 4;
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
			L.Set( PopPC() );
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
			int8 jump = (int8)PopPC();
			if ( !GetC() ) {
				uint16 addr = PC + jump;
				PC = addr;
				additionnalTicks += 4;
			}
			break;
		}
		case 0x31: {
			// LD SP, d16
			SP.Set( PopPC16() );
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
			mem->Write( HL.Get(), PopPC() );
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
			int8 jump = (int8)PopPC();
			if ( GetC() ) {
				uint16 addr = PC + jump;
				PC = addr;
				additionnalTicks += 4;
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
			A.Set( PopPC() );
			break;
		}
		case 0x3f: {
			// CCF
			SetC( !GetC() );
			SetN( false );
			SetH( false );
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
			B.Set( mem->Read( HL.Get() ) );
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
		case 0x80: {
			// ADD A,B
			Add( A, B.Get(), false );
			break;
		}
		case 0x81: {
			// ADD A,C
			Add( A, C.Get(), false );
			break;
		}
		case 0x82: {
			// ADD A,D
			Add( A, D.Get(), false );
			break;
		}
		case 0x83: {
			// ADD A,E
			Add( A, E.Get(), false );
			break;
		}
		case 0x84: {
			// ADD A,H
			Add( A, H.Get(), false );
			break;
		}
		case 0x85: {
			// ADD A,L
			Add( A, L.Get(), false );
			break;
		}
		case 0x86: {
			// ADD A,(HL)
			Add( A, mem->Read( HL.Get() ), false );
			break;
		}
		case 0x87: {
			// ADD A,A
			Add( A, A.Get(), false );
			break;
		}
		case 0x88: {
			// ADC A,B
			Add( A, B.Get(), true );
			break;
		}
		case 0x89: {
			// ADC A,C
			Add( A, C.Get(), true );
			break;
		}
		case 0x8a: {
			// ADC A,D
			Add( A, D.Get(), true );
			break;
		}
		case 0x8b: {
			// ADC A,E
			Add( A, E.Get(), true );
			break;
		}
		case 0x8c: {
			// ADC A,H
			Add( A, H.Get(), true );
			break;
		}
		case 0x8d: {
			// ADC A,L
			Add( A, L.Get(), true );
			break;
		}
		case 0x8e: {
			// ADC A,(HL)
			Add( A, mem->Read( HL.Get() ), true );
			break;
		}
		case 0x8f: {
			// ADC A,A
			Add( A, A.Get(), true );
			break;
		}
		case 0x90: {
			// SUB A, B
			Sub( A, B.Get(), false );
			break;
		}
		case 0x91: {
			// SUB A, C
			Sub( A, C.Get(), false );
			break;
		}
		case 0x92: {
			// SUB A, D
			Sub( A, D.Get(), false );
			break;
		}
		case 0x93: {
			// SUB A, E
			Sub( A, E.Get(), false );
			break;
		}
		case 0x94: {
			// SUB A, H
			Sub( A, H.Get(), false );
			break;
		}
		case 0x95: {
			// SUB A, L
			Sub( A, L.Get(), false );
			break;
		}
		case 0x96: {
			// SUB A, (HL)
			Sub( A, mem->Read( HL.Get() ), false );
			break;
		}
		case 0x97: {
			// SUB A, A
			Sub( A, A.Get(), false );
			break;
		}
		case 0x98: {
			// SBC A,B
			Sub( A, B.Get(), true );
			break;
		}
		case 0x99: {
			// SBC A,C
			Sub( A, C.Get(), true );
			break;
		}
		case 0x9a: {
			// SBC A,D
			Sub( A, D.Get(), true );
			break;
		}
		case 0x9b: {
			// SBC A,E
			Sub( A, E.Get(), true );
			break;
		}
		case 0x9c: {
			// SBC A,H
			Sub( A, H.Get(), true );
			break;
		}
		case 0x9d: {
			// SBC A,L
			Sub( A, L.Get(), true );
			break;
		}
		case 0x9e: {
			// SBC A,(HL)
			Sub( A, mem->Read( HL.Get() ), true );
			break;
		}
		case 0x9f: {
			// SBC A,A
			Sub( A, A.Get(), true );
			break;
		}
		case 0xa0: {
			// AND A, B
			And( A, B.Get() );
			break;
		}
		case 0xa1: {
			// AND A, C
			And( A, C.Get() );
			break;
		}
		case 0xa2: {
			// AND A, D
			And( A, D.Get() );
			break;
		}
		case 0xa3: {
			// AND A, E
			And( A, E.Get() );
			break;
		}
		case 0xa4: {
			// AND A, H
			And( A, H.Get() );
			break;
		}
		case 0xa5: {
			// AND A, L
			And( A, L.Get() );
			break;
		}
		case 0xa6: {
			// AND A, (HL)
			And( A, mem->Read( HL.Get() ) );
			break;
		}
		case 0xa7: {
			// AND A, A
			And( A, A.Get() );
			break;
		}
		case 0xa8: {
			// XOR A, B
			Xor( A, B.Get() );
			break;
		}
		case 0xa9: {
			// XOR A, C
			Xor( A, C.Get() );
			break;
		}
		case 0xaa: {
			// XOR A, D
			Xor( A, D.Get() );
			break;
		}
		case 0xab: {
			// XOR A, E
			Xor( A, E.Get() );
			break;
		}
		case 0xac: {
			// XOR A, H
			Xor( A, H.Get() );
			break;
		}
		case 0xad: {
			// XOR A, L
			Xor( A, L.Get() );
			break;
		}
		case 0xae: {
			// XOR A, (HL)
			Xor( A, mem->Read( HL.Get() ) );
			break;
		}
		case 0xaf: {
			// XOR A, A
			Xor( A, A.Get() );
			break;
		}
		case 0xb0: {
			// OR B
			Or( A, B.Get() );
			break;
		}
		case 0xb1: {
			// OR C
			Or( A, C.Get() );
			break;
		}
		case 0xb2: {
			// OR D
			Or( A, D.Get() );
			break;
		}
		case 0xb3: {
			// OR E
			Or( A, E.Get() );
			break;
		}
		case 0xb4: {
			// OR H
			Or( A, H.Get() );
			break;
		}
		case 0xb5: {
			// OR L
			Or( A, L.Get() );
			break;
		}
		case 0xb6: {
			// OR (HL)
			Or( A, mem->Read( HL.Get() ) );
			break;
		}
		case 0xb7: {
			// OR A
			Or( A, A.Get() );
			break;
		}
		case 0xb8: {
			// CP B
			Cp( A, B.Get() );
			break;
		}
		case 0xb9: {
			// CP C
			Cp( A, C.Get() );
			break;
		}
		case 0xba: {
			// CP D
			Cp( A, D.Get() );
			break;
		}
		case 0xbb: {
			// CP E
			Cp( A, E.Get() );
			break;
		}
		case 0xbc: {
			// CP H
			Cp( A, H.Get() );
			break;
		}
		case 0xbd: {
			// CP L
			Cp( A, L.Get() );
			break;
		}
		case 0xbe: {
			// CP (HL)
			Cp( A, mem->Read( HL.Get() ) );
			break;
		}
		case 0xbf: {
			// CP A
			Cp( A, A.Get() );
			break;
		}
		case 0xc0: {
			// RET NZ
			if ( !GetZ() ) {
				Ret();
				additionnalTicks += 12;
			}
			break;
		}
		case 0xc1: {
			// POP BC
			BC.Set( PopStack() );
			break;
		}
		case 0xc2: {
			// JP NZ, a16
			uint16 jump = PopPC16();
			if ( !GetZ() ) {
				PC = jump;
				additionnalTicks += 4;
			}
			break;
		}
		case 0xc3: {
			// JP a16
			PC = PopPC16();
			break;
		}
		case 0xc4: {
			// CALL NZ
			uint16 jump = PopPC16();
			if ( !GetZ() ) {
				Call( jump );
				additionnalTicks += 12;
			}
			break;
		}
		case 0xc5: {
			// PUSH BC
			PushStack( BC.Get() );
			break;
		}
		case 0xc6: {
			// ADD A, d8
			Add( A, PopPC(), false );
			break;
		}
		case 0xc7: {
			// RST 0x00
			Call( 0x0000 );
			break;
		}
		case 0xc8: {
			// RET Z
			if ( GetZ() ) {
				Ret();
				additionnalTicks += 12;
			}
			break;
		}
		case 0xc9: {
			// RET
			Ret();
			break;
		}
		case 0xca: {
			// JP Z, a16
			uint16 jump = PopPC16();
			if ( GetZ() ) {
				PC = jump;
				additionnalTicks += 4;
			}
			break;
		}
		case 0xcb: {
			// PREFIX CB
			additionnalTicks += ExecuteCBOPCode( this, PopPC() );
			break;
		}
		case 0xcc: {
			// CALL Z
			uint16 jump = PopPC16();
			if ( GetZ() ) {
				Call( jump );
				additionnalTicks += 12;
			}
			break;
		}
		case 0xcd: {
			// CALL a16
			Call( PopPC16() );
			break;
		}
		case 0xce: {
			// ADC A, d8
			Add( A, PopPC(), true );
			break;
		}
		case 0xcf: {
			// RST 0x08
			Call( 0x0008 );
			break;
		}
		case 0xd0: {
			// RET NC
			if ( !GetC() ) {
				Ret();
				additionnalTicks += 12;
			}
			break;
		}
		case 0xd1: {
			// POP DE
			DE.Set( PopStack() );
			break;
		}
		case 0xd2: {
			// JP NC, a16
			uint16 jump = PopPC16();
			if ( !GetC() ) {
				PC = jump;
				additionnalTicks += 4;
			}
			break;
		}
		case 0xd3: {
			INVALID_OP;
			break;
		}
		case 0xd4: {
			// CALL NC
			uint16 jump = PopPC16();
			if ( !GetC() ) {
				Call( jump );
				additionnalTicks += 12;
			}
			break;
		}
		case 0xd5: {
			// PUSH DE
			PushStack( DE.Get() );
			break;
		}
		case 0xd6: {
			// SUB A, d8
			Sub( A, PopPC(), false );
			break;
		}
		case 0xd7: {
			// RST 0x10
			Call( 0x0010 );
			break;
		}
		case 0xd8: {
			// RET C
			if ( GetC() ) {
				Ret();
				additionnalTicks += 12;
			}
			break;
		}
		case 0xd9: {
			// RETI
			Ret();
			interuptsEnabled = true;
			break;
		}
		case 0xda: {
			// JP C, a16
			uint16 jump = PopPC16();
			if ( GetC() ) {
				PC = jump;
				additionnalTicks += 4;
			}
			break;
		}
		case 0xdb: {
			INVALID_OP;
			break;
		}
		case 0xdc: {
			// CALL C
			uint16 jump = PopPC16();
			if ( GetC() ) {
				Call( jump );
				additionnalTicks += 12;
			}
			break;
		}
		case 0xdd: {
			INVALID_OP;
			break;
		}
		case 0xde: {
			// SBC A, d8
			Sub( A, PopPC(), true );
			break;
		}
		case 0xdf: {
			// RST 0x18
			Call( 0x0018 );
			break;
		}
		case 0xe0: {
			// LDH (a8), A
			mem->Write( 0xFF00 + PopPC(), A.Get() );
			break;
		}
		case 0xe1: {
			// POP HL
			HL.Set( PopStack() );
			break;
		}
		case 0xe2: {
			// LD (C), A
			mem->Write( 0xFF00 + C.Get(), A.Get() );
			break;
		}
		case 0xe3: {
			INVALID_OP;
			break;
		}
		case 0xe4: {
			INVALID_OP;
			break;
		}
		case 0xe5: {
			// PUSH HL
			PushStack( HL.Get() );
			break;
		}
		case 0xe6: {
			// AND A, d8
			And( A, PopPC() );
			break;
		}
		case 0xe7: {
			// RST 0x20
			Call( 0x0020 );
			break;
		}
		case 0xe8: {
			// ADD SP, r8
			Add16Signed( SP, PopPC() );
			SetZ( false );
			break;
		}
		case 0xe9: {
			// JP HL
			PC = HL.Get();
			break;
		}
		case 0xea: {
			// LD (a16), A
			mem->Write( PopPC16(), A.Get() );
			break;
		}
		case 0xeb: {
			INVALID_OP;
			break;
		}
		case 0xec: {
			INVALID_OP;
			break;
		}
		case 0xed: {
			INVALID_OP;
			break;
		}
		case 0xee: {
			// XOR A, d8
			Xor( A, PopPC() );
			break;
		}
		case 0xef: {
			// RST 0x28
			Call( 0x0028 );
			break;
		}
		case 0xf0: {
			// LDH A, (a8)
			A.Set( mem->Read( 0xFF00 + PopPC() ) );
			break;
		}
		case 0xf1: {
			// TODO: Apparently this might affect flags? But I have no idea how
			// POP AF
			AF.Set( PopStack() );
			break;
		}
		case 0xf2: {
			// LD A, (C)
			A.Set( mem->Read( 0xFF00 + C.Get() ) );
			break;
		}
		case 0xf3: {
			// DI
			interuptsOn = false;
			break;
		}
		case 0xf4: {
			INVALID_OP;
			break;
		}
		case 0xf5: {
			// PUSH AF
			PushStack( AF.Get() );
			break;
		}
		case 0xf6: {
			// OR A, d8
			Or( A, PopPC() );
			break;
		}
		case 0xf7: {
			// RST 0x30
			Call( 0x0030 );
			break;
		}
		case 0xf8: {
			// LD HL, SP+r8
			HL.Set( SP.Get() );
			Add16Signed( HL, PopPC() );
			break;
		}
		case 0xf9: {
			// LD SP, HL
			SP.Set( HL.Get() );
			break;
		}
		case 0xfa: {
			// LD A, (a16)
			A.Set( mem->Read( PopPC16() ) );
			break;
		}
		case 0xfb: {
			// EI
			interuptsEnabled = true;
			break;
		}
		case 0xfc: {
			INVALID_OP;
			break;
		}
		case 0xfd: {
			INVALID_OP;
			break;
		}
		case 0xfe: {
			// CP A, d8
			Cp( A, PopPC() );
			break;
		}
		case 0xff: {
			// RST 0x38
			Call( 0x0038 );
		}
	}
}
