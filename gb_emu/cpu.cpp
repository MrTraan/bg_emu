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
	// All instructions are detailled here : https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html
	byte opcode	   = PopPC( mem );
	int  ticksUsed = opcodeCyclesCost[ opcode ] * 4;
	additionnalTicks = 0;

	InstructionPtr op = s_instructions[ opcode ];
	if ( op == nullptr ) {
		// Unknown opcode
		DEBUG_BREAK;
	} else {
		( this->*op )();
	}

	return ticksUsed + additionnalTicks;
}

void Cpu::Inst0x0() {
	// NOP
}

void Cpu::Inst0x01() {
	// LD BC, d16
	uint16 val = PopPC16( mem );
	BC.Set( val );
}

void Cpu::Inst0x02() {
	// LD (BC), A
	uint16 addr = BC.Get();
	byte   val  = A.Get();
	mem->Write( addr, val );
}

void Cpu::Inst0x03() {
	// INC BC
	Inc16( BC );
}

void Cpu::Inst0x04() {
	// INC B
	Inc( B );
}

void Cpu::Inst0x05() {
	// DEC B
	Dec( B );
}

void Cpu::Inst0x06() {
	// LD B, d8
	byte val = PopPC( mem );
	B.Set( val );
}

void Cpu::Inst0x07() {
	// RLCA
	byte val = A.Get();
	byte res = ( val << 1 ) || ( val >> 7 ); // Put last bit first
	A.Set( res );
	SetZ( false );
	SetN( false );
	SetH( false );
	SetC( val > 0x7f );
}

void Cpu::Inst0x08() {
	// LD (a16), SP
	uint16 addr = PopPC16( mem );
	mem->Write( addr, SP.low.Get() );
	mem->Write( addr + 1, SP.high.Get() );
}

void Cpu::Inst0x09() {
	// ADD HL, BC
	Add16( HL, BC.Get() );
}

void Cpu::Inst0x0a() {
	// LD A, (BC)
	byte val = mem->Read( BC.Get() );
	A.Set( val );
}

void Cpu::Inst0x0b() {
	// DEC BC
	Dec16( BC );
}

void Cpu::Inst0x0c() {
	// INC C
	Inc( C );
}

void Cpu::Inst0x0d() {
	// DEC C
	Dec( C );
}

void Cpu::Inst0x0e() {
	// LD C, d8
	byte val = PopPC( mem );
	C.Set( val );
}

void Cpu::Inst0x0f() {
	// RRCA
	byte val = A.Get();
	byte res = ( val >> 1 ) | ( ( val & 1 ) << 7 );
	A.Set( res );
	SetZ( false );
	SetN( false );
	SetH( false );
	SetC( res > 0x7f );
}

void Cpu::Inst0x10() {
	// STOP 0
	Halt();
	PopPC( mem ); // Read next byte, because this instruction is actualy 2 bytes : 0x10 0x00, no idea why
}

void Cpu::Inst0x11() {
	// LD DE, d16
	BC.Set( PopPC16( mem ) );
}

void Cpu::Inst0x12() {
	// LD (DE), A
	mem->Write( DE.Get(), A.Get() );
}

void Cpu::Inst0x13() {
	// INC DE
	Inc16( DE );
}

void Cpu::Inst0x14() {
	// INC D
	Inc( D );
}

void Cpu::Inst0x15() {
	// DEC D
	Dec( D );
}

void Cpu::Inst0x16() {
	// LD D, d8
	D.Set( PopPC( mem ) );
}

void Cpu::Inst0x17() {
	// RLA
	byte val   = A.Get();
	byte carry = GetC() ? 1 : 0;
	byte res   = ( val << 1 ) + carry;
	A.Set( res );
	SetZ( false );
	SetN( false );
	SetH( false );
	SetC( res > 0x7f );
}

void Cpu::Inst0x18() {
	// JR r8
	uint16 addr = PC + PopPC( mem );
	PC			= addr;
}

void Cpu::Inst0x19() {
	// ADD HL, DE
	Add16( HL, DE.Get() );
}

void Cpu::Inst0x1a() {
	// LD A, (DE)
	A.Set( mem->Read( DE.Get() ) );
}

void Cpu::Inst0x1b() {
	// DEC DE
	Dec16( DE );
}

void Cpu::Inst0x1c() {
	// INC E
	Inc( E );
}

void Cpu::Inst0x1d() {
	// DEC E
	Dec( E );
}

void Cpu::Inst0x1e() {
	// LD E, d8
	E.Set( PopPC( mem ) );
}

void Cpu::Inst0x1f() {
	// RRA
	byte val   = A.Get();
	byte carry = GetC() ? 0x80 : 0;
	byte res   = ( val >> 1 ) | carry;
	A.Set( res );
	SetZ( false );
	SetN( false );
	SetH( false );
	SetC( ( 1 & val ) == 1 );
}

void Cpu::Inst0x20() {
	// JR NZ, r8
	if ( !GetZ() ) {
		uint16 addr = PC + PopPC( mem );
		PC			= addr;
		additionnalTicks += 4;
	}
}

void Cpu::Inst0x21() {
	// LD HL, d16
	HL.Set( PopPC16( mem ) );
}

void Cpu::Inst0x22() {
	// LD (HL+), A
	mem->Write( HL.Get(), A.Get() );
	Inc16( HL );
}

void Cpu::Inst0x23() {
	// INC HL
	Inc16( HL );
}

void Cpu::Inst0x24() {
	// INC H
	Inc( H );
}

void Cpu::Inst0x25() {
	// DEC H
	Dec( H );
}

void Cpu::Inst0x26() {
	// LD H, d8
	H.Set( PopPC( mem ) );
}

void Cpu::Inst0x27() {
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
}

void Cpu::Inst0x28() {
	// JR Z, r8
	if ( GetZ() ) {
		uint16 addr = PC + PopPC( mem );
		PC			= addr;
		additionnalTicks += 4;
	}
}

void Cpu::Inst0x29() {
	// ADD HL, HL
	Add16( HL, HL.Get() );
}

void Cpu::Inst0x2a() {
	// LD A, (HL+)
	A.Set( mem->Read( HL.Get() ) );
	Inc16( HL );
}

void Cpu::Inst0x2b() {
	// DEC HL
	Dec16( HL );
}

void Cpu::Inst0x2c() {
	// INC L
	Inc( L );
}

void Cpu::Inst0x2d() {
	// DEC L
	Dec( L );
}

void Cpu::Inst0x2e() {
	// LD L, d8
	L.Set( PopPC( mem ) );
}

void Cpu::Inst0x2f() {
	// CPL
	A.Set( 0xFF ^ A.Get() );
	SetN( true );
	SetH( true );
}

void Cpu::Inst0x30() {
	// JR NC, r8
	if ( !GetC() ) {
		uint16 addr = PC + PopPC( mem );
		PC			= addr;
		additionnalTicks += 4;
	}
}

void Cpu::Inst0x31() {
	// LD SP, d16
	SP.Set( PopPC16( mem ) );
}

void Cpu::Inst0x32() {
	// LD (HL-), A
	mem->Write( HL.Get(), A.Get() );
	Dec16( HL );
}

void Cpu::Inst0x33() {
	// INC SP
	Inc16( SP );
}

void Cpu::Inst0x34() {
	// INC (HL)
	// We are using A as a temporary register for the value pointed to by HL
	byte previousA = A.Get();
	A.Set( mem->Read( HL.Get() ) );
	Inc( A );
	mem->Write( HL.Get(), A.Get() );
	A.Set( previousA );
}

void Cpu::Inst0x35() {
	// DEC (HL)
	// We are using A as a temporary register for the value pointed to by HL
	byte previousA = A.Get();
	A.Set( mem->Read( HL.Get() ) );
	Dec( A );
	mem->Write( HL.Get(), A.Get() );
	A.Set( previousA );
}

void Cpu::Inst0x36() {
	// LD (HL), d8
	mem->Write( HL.Get(), PopPC( mem ) );
}

void Cpu::Inst0x37() {
	// SCF
	SetN( false );
	SetH( false );
	SetC( true );
}

void Cpu::Inst0x38() {
	// JR C, r8
	if ( GetC() ) {
		uint16 addr = PC + PopPC( mem );
		PC			= addr;
		additionnalTicks += 4;
	}
}

void Cpu::Inst0x39() {
	// ADD HL, SP
	Add16( HL, SP.Get() );
}

void Cpu::Inst0x3a() {
	// LD A, (HL-)
	A.Set( mem->Read( HL.Get() ) );
	Dec16( HL );
}

void Cpu::Inst0x3b() {
	// DEC SP
	Dec16( SP );
}

void Cpu::Inst0x3c() {
	// INC A
	Inc( A );
}

void Cpu::Inst0x3d() {
	// DEC A
	Dec( A );
}

void Cpu::Inst0x3e() {
	// LD A, d8
	A.Set( PopPC( mem ) );
}

void Cpu::Inst0x3f() {
	// CCF
	SetN( false );
	SetH( false );
	SetC( !GetC() );
}

void Cpu::Inst0x40() {
	// LD B, B
	B.Set( B.Get() );
}

void Cpu::Inst0x41() {
	// LD B, C
	B.Set( C.Get() );
}

void Cpu::Inst0x42() {
	// LD B, D
	B.Set( D.Get() );
}

void Cpu::Inst0x43() {
	// LD B, E
	B.Set( E.Get() );
}

void Cpu::Inst0x44() {
	// LD B, H
	B.Set( H.Get() );
}

void Cpu::Inst0x45() {
	// LD B, L
	B.Set( L.Get() );
}

void Cpu::Inst0x46() {
	// LD B, (HL)
	B.Set( mem->Read( HL.Get() ) );
}

void Cpu::Inst0x47() {
	// LD B, A
	B.Set( A.Get() );
}

void Cpu::Inst0x48() {
	// LD C, B
	C.Set( B.Get() );
}

void Cpu::Inst0x49() {
	// LD C, C
	C.Set( C.Get() );
}

void Cpu::Inst0x4a() {
	// LD C, D
	C.Set( D.Get() );
}

void Cpu::Inst0x4b() {
	// LD C, E
	C.Set( E.Get() );
}

void Cpu::Inst0x4c() {
	// LD C, H
	C.Set( H.Get() );
}

void Cpu::Inst0x4d() {
	// LD C, L
	C.Set( L.Get() );
}

void Cpu::Inst0x4e() {
	// LD C, (HL)
	C.Set( mem->Read( HL.Get() ) );
}

void Cpu::Inst0x4f() {
	// LD C, A
	C.Set( A.Get() );
}

void Cpu::Inst0x50() {
	// LD D, B
	D.Set( B.Get() );
}

void Cpu::Inst0x51() {
	// LD D, C
	D.Set( C.Get() );
}

void Cpu::Inst0x52() {
	// LD D, D
	D.Set( D.Get() );
}

void Cpu::Inst0x53() {
	// LD D, E
	D.Set( E.Get() );
}

void Cpu::Inst0x54() {
	// LD D, H
	D.Set( H.Get() );
}

void Cpu::Inst0x55() {
	// LD D, L
	D.Set( L.Get() );
}

void Cpu::Inst0x56() {
	// LD D, (HL)
	D.Set( mem->Read( HL.Get() ) );
}

void Cpu::Inst0x57() {
	// LD D, A
	D.Set( A.Get() );
}

void Cpu::Inst0x58() {
	// LD E, B
	E.Set( B.Get() );
}

void Cpu::Inst0x59() {
	// LD E, C
	E.Set( C.Get() );
}

void Cpu::Inst0x5a() {
	// LD E, D
	E.Set( D.Get() );
}

void Cpu::Inst0x5b() {
	// LD E, E
	E.Set( E.Get() );
}

void Cpu::Inst0x5c() {
	// LD E, H
	E.Set( H.Get() );
}

void Cpu::Inst0x5d() {
	// LD E, L
	E.Set( L.Get() );
}

void Cpu::Inst0x5e() {
	// LD E, (HL)
	E.Set( mem->Read( HL.Get() ) );
}

void Cpu::Inst0x5f() {
	// LD E, A
	E.Set( A.Get() );
}

void Cpu::Inst0x60() {
	// LD H, B
	H.Set( B.Get() );
}

void Cpu::Inst0x61() {
	// LD H, C
	H.Set( C.Get() );
}

void Cpu::Inst0x62() {
	// LD H, D
	H.Set( D.Get() );
}

void Cpu::Inst0x63() {
	// LD H, E
	H.Set( E.Get() );
}

void Cpu::Inst0x64() {
	// LD H, H
	H.Set( H.Get() );
}

void Cpu::Inst0x65() {
	// LD H, L
	H.Set( L.Get() );
}

void Cpu::Inst0x66() {
	// LD H, (HL)
	H.Set( mem->Read( HL.Get() ) );
}

void Cpu::Inst0x67() {
	// LD H, A
	H.Set( A.Get() );
}

void Cpu::Inst0x68() {
	// LD L, B
	L.Set( B.Get() );
}

void Cpu::Inst0x69() {
	// LD L, C
	L.Set( C.Get() );
}

void Cpu::Inst0x6a() {
	// LD L, D
	L.Set( D.Get() );
}

void Cpu::Inst0x6b() {
	// LD L, E
	L.Set( E.Get() );
}

void Cpu::Inst0x6c() {
	// LD L, H
	L.Set( H.Get() );
}

void Cpu::Inst0x6d() {
	// LD L, L
	L.Set( L.Get() );
}

void Cpu::Inst0x6e() {
	// LD L, (HL)
	L.Set( mem->Read( HL.Get() ) );
}

void Cpu::Inst0x6f() {
	// LD L, A
	L.Set( A.Get() );
}

void Cpu::Inst0x70() {
	// LD (HL), B
	mem->Write( HL.Get(), B.Get() );
}

void Cpu::Inst0x71() {
	// LD (HL), C
	mem->Write( HL.Get(), C.Get() );
}

void Cpu::Inst0x72() {
	// LD (HL), D
	mem->Write( HL.Get(), D.Get() );
}

void Cpu::Inst0x73() {
	// LD (HL), E
	mem->Write( HL.Get(), E.Get() );
}

void Cpu::Inst0x74() {
	// LD (HL), H
	mem->Write( HL.Get(), H.Get() );
}

void Cpu::Inst0x75() {
	// LD (HL), L
	mem->Write( HL.Get(), L.Get() );
}

void Cpu::Inst0x76() {
	// HALT
	Halt();
}

void Cpu::Inst0x77() {
	// LD (HL), A
	mem->Write( HL.Get(), A.Get() );
}

void Cpu::Inst0x78() {
	// LD A, B
	A.Set( B.Get() );
}

void Cpu::Inst0x79() {
	// LD A, C
	A.Set( C.Get() );
}

void Cpu::Inst0x7a() {
	// LD A, D
	A.Set( D.Get() );
}

void Cpu::Inst0x7b() {
	// LD A, E
	A.Set( E.Get() );
}

void Cpu::Inst0x7c() {
	// LD A, H
	A.Set( H.Get() );
}

void Cpu::Inst0x7d() {
	// LD A, L
	A.Set( L.Get() );
}

void Cpu::Inst0x7e() {
	// LD A, (HL)
	A.Set( mem->Read( HL.Get() ) );
}

void Cpu::Inst0x7f() {
	// LD A, A
	A.Set( A.Get() );
}

Cpu::InstructionPtr Cpu::s_instructions[0xff] = {
		&Cpu::Inst0x0,
		&Cpu::Inst0x01,
		&Cpu::Inst0x02,
		&Cpu::Inst0x03,
		&Cpu::Inst0x04,
		&Cpu::Inst0x05,
		&Cpu::Inst0x06,
		&Cpu::Inst0x07,
		&Cpu::Inst0x08,
		&Cpu::Inst0x09,
		&Cpu::Inst0x0a,
		&Cpu::Inst0x0b,
		&Cpu::Inst0x0c,
		&Cpu::Inst0x0d,
		&Cpu::Inst0x0e,
		&Cpu::Inst0x0f,
		&Cpu::Inst0x10,
		&Cpu::Inst0x11,
		&Cpu::Inst0x12,
		&Cpu::Inst0x13,
		&Cpu::Inst0x14,
		&Cpu::Inst0x15,
		&Cpu::Inst0x16,
		&Cpu::Inst0x17,
		&Cpu::Inst0x18,
		&Cpu::Inst0x19,
		&Cpu::Inst0x1a,
		&Cpu::Inst0x1b,
		&Cpu::Inst0x1c,
		&Cpu::Inst0x1d,
		&Cpu::Inst0x1e,
		&Cpu::Inst0x1f,
		&Cpu::Inst0x20,
		&Cpu::Inst0x21,
		&Cpu::Inst0x22,
		&Cpu::Inst0x23,
		&Cpu::Inst0x24,
		&Cpu::Inst0x25,
		&Cpu::Inst0x26,
		&Cpu::Inst0x27,
		&Cpu::Inst0x28,
		&Cpu::Inst0x29,
		&Cpu::Inst0x2a,
		&Cpu::Inst0x2b,
		&Cpu::Inst0x2c,
		&Cpu::Inst0x2d,
		&Cpu::Inst0x2e,
		&Cpu::Inst0x2f,
		&Cpu::Inst0x30,
		&Cpu::Inst0x31,
		&Cpu::Inst0x32,
		&Cpu::Inst0x33,
		&Cpu::Inst0x34,
		&Cpu::Inst0x35,
		&Cpu::Inst0x36,
		&Cpu::Inst0x37,
		&Cpu::Inst0x38,
		&Cpu::Inst0x39,
		&Cpu::Inst0x3a,
		&Cpu::Inst0x3b,
		&Cpu::Inst0x3c,
		&Cpu::Inst0x3d,
		&Cpu::Inst0x3e,
		&Cpu::Inst0x3f,
		&Cpu::Inst0x40,
		&Cpu::Inst0x41,
		&Cpu::Inst0x42,
		&Cpu::Inst0x43,
		&Cpu::Inst0x44,
		&Cpu::Inst0x45,
		&Cpu::Inst0x46,
		&Cpu::Inst0x47,
		&Cpu::Inst0x48,
		&Cpu::Inst0x49,
		&Cpu::Inst0x4a,
		&Cpu::Inst0x4b,
		&Cpu::Inst0x4c,
		&Cpu::Inst0x4d,
		&Cpu::Inst0x4e,
		&Cpu::Inst0x4f,
		&Cpu::Inst0x50,
		&Cpu::Inst0x51,
		&Cpu::Inst0x52,
		&Cpu::Inst0x53,
		&Cpu::Inst0x54,
		&Cpu::Inst0x55,
		&Cpu::Inst0x56,
		&Cpu::Inst0x57,
		&Cpu::Inst0x58,
		&Cpu::Inst0x59,
		&Cpu::Inst0x5a,
		&Cpu::Inst0x5b,
		&Cpu::Inst0x5c,
		&Cpu::Inst0x5d,
		&Cpu::Inst0x5e,
		&Cpu::Inst0x5f,
		&Cpu::Inst0x60,
		&Cpu::Inst0x61,
		&Cpu::Inst0x62,
		&Cpu::Inst0x63,
		&Cpu::Inst0x64,
		&Cpu::Inst0x65,
		&Cpu::Inst0x66,
		&Cpu::Inst0x67,
		&Cpu::Inst0x68,
		&Cpu::Inst0x69,
		&Cpu::Inst0x6a,
		&Cpu::Inst0x6b,
		&Cpu::Inst0x6c,
		&Cpu::Inst0x6d,
		&Cpu::Inst0x6e,
		&Cpu::Inst0x6f,
		&Cpu::Inst0x70,
		&Cpu::Inst0x71,
		&Cpu::Inst0x72,
		&Cpu::Inst0x73,
		&Cpu::Inst0x74,
		&Cpu::Inst0x75,
		&Cpu::Inst0x76,
		&Cpu::Inst0x77,
		&Cpu::Inst0x78,
		&Cpu::Inst0x79,
		&Cpu::Inst0x7a,
		&Cpu::Inst0x7b,
		&Cpu::Inst0x7c,
		&Cpu::Inst0x7d,
		&Cpu::Inst0x7e,
		&Cpu::Inst0x7f,
};