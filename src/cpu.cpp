#include <stdio.h>
#include "cpu.h"
#include "cb_opcodes.h"
#include "memory.h"

#define INVALID_OP DEBUG_BREAK

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

// Return Cycles used
int Cpu::ExecuteNextOPCode() {
	// All instructions are detailled here : https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html
	byte opcode = PopPC();
	int  ticksUsed = opcodeCyclesCost[opcode] * 4;
	additionnalTicks = 0;
	lastInstructionName = s_instructionsNames[opcode];
	//printf("%s\n", s_instructionsNames[opcode]);
	//static bool startLogging = false;
	//if ( PC - 1 == 0x100 ) {
	//	startLogging = true;
	//}
	//if (startLogging)
	//	printf("0x%02x 0x%04x\n", opcode, PC - 1);

	InstructionPtr op = s_instructions[opcode];
	if (op == nullptr) {
		// Unknown opcode
		DEBUG_BREAK;
	}
	else {
		(this->*op)();
	}
	return ticksUsed + additionnalTicks;
}

void Cpu::UpdateTimer(int clock) {
	divider += clock;
	if (divider >= 255) {
		divider -= 255;
		byte div = mem->Read(DIV);
		if (div == 255)
			mem->highRAM[DIV - 0xFF00] = 0;
		else
			mem->highRAM[DIV - 0xFF00]++;
	}

	byte tac = mem->highRAM[TAC - 0xFF00];
	byte frequency = tac & 3;
	constexpr int threshold[4] = {1024, 16, 64, 256};

	if (!BIT_IS_SET(tac, 2))
		return;

	clockCounter += clock;
	if (clockCounter > threshold[frequency])
	{
		clockCounter -= threshold[frequency];
		byte tima = mem->highRAM[TIMA - 0xFF00];
		if (tima == 0xFF)
		{
			mem->highRAM[TIMA - 0xFF00] = mem->highRAM[TMA-0xFF00];
			RaiseInterupt(2);
		}
		else
		{
			mem->highRAM[TIMA - 0xFF00]++;
		}
	}
}

void Cpu::Add(Register8& reg, byte val, bool useCarry) {
	byte  valReg = reg.Get();
	byte  carry = GetC() && useCarry ? 1 : 0;
	int16 total = valReg + val + carry;
	reg.Set((byte)total);

	SetZ((byte)total == 0);
	SetN(false);
	SetH((val & 0xF) + (valReg & 0xF) + carry > 0xF);
	SetC(total > 0xFF);
}

void Cpu::Sub(Register8& reg, byte val, bool useCarry) {
	byte  valReg = reg.Get();
	byte  carry = GetC() && useCarry ? 1 : 0;
	int16 total = valReg - val - carry;
	reg.Set((byte)total);

	SetZ((byte)total == 0);
	SetN(true);
	SetH((int16)(valReg & 0xF) - (val & 0xF) - carry < 0);
	SetC(total < 0);
}

void Cpu::And(Register8& reg, byte val) {
	byte valReg = reg.Get();
	byte total = valReg & val;
	reg.Set(total);

	SetZ(total == 0);
	SetN(false);
	SetH(true);
	SetC(false);
}

void Cpu::Or(Register8& reg, byte val) {
	byte valReg = reg.Get();
	byte total = valReg | val;
	reg.Set(total);

	SetZ(total == 0);
	SetN(false);
	SetH(false);
	SetC(false);
}

void Cpu::Xor(Register8& reg, byte val) {
	byte valReg = reg.Get();
	byte total = valReg ^ val;
	reg.Set(total);

	SetZ(total == 0);
	SetN(false);
	SetH(false);
	SetC(false);
}

void Cpu::Cp(Register8& reg, byte val) {
	byte valReg = reg.Get();
	byte total = val - valReg;
	SetZ(total == 0);
	SetN(true);
	SetH((valReg & 0x0f) < (val & 0x0f));
	SetC(valReg < val);
}

void Cpu::Inc(Register8& reg) {
	byte valReg = reg.Get();
	byte total = valReg + 1;
	reg.Set(total);
	SetZ(total == 0);
	SetN(false);
	SetH((valReg & 0x0f) + 1 > 0x0f);
}

void Cpu::Dec(Register8& reg) {
	byte valReg = reg.Get();
	byte total = valReg - 1;
	reg.Set(total);
	SetZ(total == 0);
	SetN(true);
	SetH((valReg & 0x0f) == 0x0);
}

void Cpu::Add16(Register16& reg, uint16 val) {
	uint16 valReg = reg.Get();
	int	   total = valReg + val;
	reg.Set((uint16)total);
	SetN(false);
	SetH((int)(valReg & 0xfff) > (total & 0xfff));
	SetC(total > 0xffff);
}

void Cpu::Add16Signed(Register16& reg, int8 val) {
	uint16 valReg = reg.Get();
	uint16 total = (int)valReg + (int)val;
	reg.Set(total);
	SetZ(false);
	SetN(false);
	SetH(((valReg ^ val ^ total) & 0x10) == 0x10);
	SetC(((valReg ^ val ^ total) & 0x100) == 0x100);
}

void Cpu::Inc16(Register16& reg) {
	reg.Set(reg.Get() + 1);
}

void Cpu::Dec16(Register16& reg) {
	reg.Set(reg.Get() - 1);
}

// Saves the current execution address on the stack before moving PC the requested address
void Cpu::Call(uint16 addr) {
	PushStack(PC);
	PC = addr;
}

// Returns by setting PC value to the value on the stack
void Cpu::Ret() {
	PC = PopStack();
}

byte Cpu::PopPC() {
	byte opcode = mem->Read(PC);
	PC++;
	return opcode;
}

uint16 Cpu::PopPC16() {
	byte val1 = mem->Read(PC);
	PC++;
	byte val2 = mem->Read(PC);
	PC++;
	return ((uint16)val2 << 8) | val1;
}

// Pushes a 16 bit value on the stack, and decrement the stack pointer
void Cpu::PushStack(uint16 val) {
	mem->Write(SP.Get() - 1, BIT_HIGH_8(val));
	mem->Write(SP.Get() - 2, BIT_LOW_8(val));
	SP.Set(SP.Get() - 2);
}

// Read the 16 bits on the stack, and increment the stack pointer
uint16 Cpu::PopStack() {
	byte b1 = mem->Read(SP.Get());
	byte b2 = mem->Read(SP.Get() + 1);
	SP.Set(SP.Get() + 2);
	return ((uint16)b2 << 8) | b1;
}

void Cpu::Halt() {
	isOnHalt = true;
}

void Cpu::RaiseInterupt(byte code) {
	byte mask = mem->Read(0xff0f);
	mask = BIT_SET(mask, code);
	mem->Write(0xff0f, mask);
}

static uint16 interruptAddresses[] = {
	0x40, // V-Blank
	0x48, // LCDC Status
	0x50, // Timer Overflow
	0x58, // Serial Transfer
	0x60, // Hi-Lo P10-P13
};

int Cpu::ProcessInterupts() {
	if (interuptsEnabled) {
		interuptsOn = true;
		interuptsEnabled = false;
		return 0;
	}
	if (!interuptsOn && !isOnHalt) {
		return 0;
	}

	byte mask = mem->Read(0xff0f);
	byte enabledMask = mem->Read(0xffff);

	for (byte i = 0; i < 5; i++) {
		if (BIT_IS_SET(mask, i) && BIT_IS_SET(enabledMask, i)) {
			if (!interuptsOn && isOnHalt) {
				isOnHalt = false;
			} else {
				interuptsOn = false;
				isOnHalt = false;
				mask = BIT_UNSET(mask, i);
				mem->Write(0xff0f, mask);

				PushStack(PC);
				PC = interruptAddresses[i];
			}
			return 20;
		}
	}

	return 0;
}

void Cpu::Inst0x00() {
	// NOP
}
void Cpu::Inst0x01() {
	// LD BC, d16
	uint16 val = PopPC16();
	BC.Set(val);
}
void Cpu::Inst0x02() {
	// LD (BC), A
	uint16 addr = BC.Get();
	byte   val = A.Get();
	mem->Write(addr, val);
}
void Cpu::Inst0x03() {
	// INC BC
	Inc16(BC);
}
void Cpu::Inst0x04() {
	// INC B
	Inc(B);
}
void Cpu::Inst0x05() {
	// DEC B
	Dec(B);
}
void Cpu::Inst0x06() {
	// LD B, d8
	byte val = PopPC();
	B.Set(val);
}
void Cpu::Inst0x07() {
	// RLCA
	byte val = A.Get();
	byte res = (val << 1) | (val >> 7); // Put last bit first
	A.Set(res);
	SetZ(false);
	SetN(false);
	SetH(false);
	SetC(val > 0x7f);
}
void Cpu::Inst0x08() {
	// LD (a16), SP
	uint16 addr = PopPC16();
	mem->Write(addr, SP.low.Get());
	mem->Write(addr + 1, SP.high.Get());
}
void Cpu::Inst0x09() {
	// ADD HL, BC
	Add16(HL, BC.Get());
}
void Cpu::Inst0x0a() {
	// LD A, (BC)
	byte val = mem->Read(BC.Get());
	A.Set(val);
}
void Cpu::Inst0x0b() {
	// DEC BC
	Dec16(BC);
}
void Cpu::Inst0x0c() {
	// INC C
	Inc(C);
}
void Cpu::Inst0x0d() {
	// DEC C
	Dec(C);
}
void Cpu::Inst0x0e() {
	// LD C, d8
	byte val = PopPC();
	C.Set(val);
}
void Cpu::Inst0x0f() {
	// RRCA
	byte val = A.Get();
	byte res = (val >> 1) | ((val & 1) << 7);
	A.Set(res);
	SetZ(false);
	SetN(false);
	SetH(false);
	SetC(res > 0x7f);
}
void Cpu::Inst0x10() {
	// STOP 0
	//Halt();
	PopPC(); // Read next byte, because this instruction is actualy 2 bytes : 0x10 0x00, no idea why
	// TODO: Game boy color change speed
}
void Cpu::Inst0x11() {
	// LD DE, d16
	DE.Set(PopPC16());
}
void Cpu::Inst0x12() {
	// LD (DE), A
	mem->Write(DE.Get(), A.Get());
}
void Cpu::Inst0x13() {
	// INC DE
	Inc16(DE);
}
void Cpu::Inst0x14() {
	// INC D
	Inc(D);
}
void Cpu::Inst0x15() {
	// DEC D
	Dec(D);
}
void Cpu::Inst0x16() {
	// LD D, d8
	D.Set(PopPC());
}
void Cpu::Inst0x17() {
	// RLA
	byte val = A.Get();
	byte carry = GetC() ? 1 : 0;
	byte res = (val << 1) + carry;
	A.Set(res);
	SetZ(false);
	SetN(false);
	SetH(false);
	SetC(val > 0x7f);
}
void Cpu::Inst0x18() {
	// JR r8
	int addr = (int8)PopPC() + (int)PC;
	PC = (uint16)addr;
}
void Cpu::Inst0x19() {
	// ADD HL, DE
	Add16(HL, DE.Get());
}
void Cpu::Inst0x1a() {
	// LD A, (DE)
	A.Set(mem->Read(DE.Get()));
}
void Cpu::Inst0x1b() {
	// DEC DE
	Dec16(DE);
}
void Cpu::Inst0x1c() {
	// INC E
	Inc(E);
}
void Cpu::Inst0x1d() {
	// DEC E
	Dec(E);
}
void Cpu::Inst0x1e() {
	// LD E, d8
	E.Set(PopPC());
}
void Cpu::Inst0x1f() {
	// RRA
	byte val = A.Get();
	byte carry = GetC() ? 0x80 : 0;
	byte res = (val >> 1) | carry;
	A.Set(res);
	SetZ(false);
	SetN(false);
	SetH(false);
	SetC((1 & val) == 1);
}
void Cpu::Inst0x20() {
	// JR NZ, r8
	int8 jump = (int8)PopPC();
	if (!GetZ()) {
		uint16 addr = PC + jump;
		PC = addr;
		additionnalTicks += 4;
	}
}
void Cpu::Inst0x21() {
	// LD HL, d16
	HL.Set(PopPC16());
}
void Cpu::Inst0x22() {
	// LD (HL+), A
	mem->Write(HL.Get(), A.Get());
	Inc16(HL);
}
void Cpu::Inst0x23() {
	// INC HL
	Inc16(HL);
}
void Cpu::Inst0x24() {
	// INC H
	Inc(H);
}
void Cpu::Inst0x25() {
	// DEC H
	Dec(H);
}
void Cpu::Inst0x26() {
	// LD H, d8
	H.Set(PopPC());
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
	if (!GetN()) {
		if (GetC() || A.Get() > 0x99) {
			A.Set(A.Get() + 0x60);
			SetC(true);
		}
		if (GetH() || (A.Get() & 0xF) > 0x9) {
			A.Set(A.Get() + 0x06);
			SetH(false);
		}
	}
	else if (GetC() && GetH()) {
		A.Set(A.Get() + 0x9a);
		SetH(false);
	}
	else if (GetC()) {
		A.Set(A.Get() + 0xa0);
	}
	else if (GetH()) {
		A.Set(A.Get() + 0xfa);
		SetH(false);
	}
	SetZ(A.Get() == 0);
}
void Cpu::Inst0x28() {
	// JR Z, r8
	int8 jump = (int8)PopPC();
	if (GetZ()) {
		uint16 addr = PC + jump;
		PC = addr;
		additionnalTicks += 4;
	}
}
void Cpu::Inst0x29() {
	// ADD HL, HL
	Add16(HL, HL.Get());
}
void Cpu::Inst0x2a() {
	// LD A, (HL+)
	A.Set(mem->Read(HL.Get()));
	Inc16(HL);
}
void Cpu::Inst0x2b() {
	// DEC HL
	Dec16(HL);
}
void Cpu::Inst0x2c() {
	// INC L
	Inc(L);
}
void Cpu::Inst0x2d() {
	// DEC L
	Dec(L);
}
void Cpu::Inst0x2e() {
	// LD L, d8
	L.Set(PopPC());
}
void Cpu::Inst0x2f() {
	// CPL
	A.Set(0xFF ^ A.Get());
	SetN(true);
	SetH(true);
}
void Cpu::Inst0x30() {
	// JR NC, r8
	int8 jump = (int8)PopPC();
	if (!GetC()) {
		uint16 addr = PC + jump;
		PC = addr;
		additionnalTicks += 4;
	}
}
void Cpu::Inst0x31() {
	// LD SP, d16
	SP.Set(PopPC16());
}
void Cpu::Inst0x32() {
	// LD (HL-), A
	mem->Write(HL.Get(), A.Get());
	Dec16(HL);
}
void Cpu::Inst0x33() {
	// INC SP
	Inc16(SP);
}
void Cpu::Inst0x34() {
	// INC (HL)
	// We are using A as a temporary register for the value pointed to by HL
	byte previousA = A.Get();
	A.Set(mem->Read(HL.Get()));
	Inc(A);
	mem->Write(HL.Get(), A.Get());
	A.Set(previousA);
}
void Cpu::Inst0x35() {
	// DEC (HL)
	// We are using A as a temporary register for the value pointed to by HL
	byte previousA = A.Get();
	A.Set(mem->Read(HL.Get()));
	Dec(A);
	mem->Write(HL.Get(), A.Get());
	A.Set(previousA);
}
void Cpu::Inst0x36() {
	// LD (HL), d8
	mem->Write(HL.Get(), PopPC());
}
void Cpu::Inst0x37() {
	// SCF
	SetN(false);
	SetH(false);
	SetC(true);
}
void Cpu::Inst0x38() {
	// JR C, r8
	int8 jump = (int8)PopPC();
	if (GetC()) {
		uint16 addr = PC + jump;
		PC = addr;
		additionnalTicks += 4;
	}
}
void Cpu::Inst0x39() {
	// ADD HL, SP
	Add16(HL, SP.Get());
}
void Cpu::Inst0x3a() {
	// LD A, (HL-)
	A.Set(mem->Read(HL.Get()));
	Dec16(HL);
}
void Cpu::Inst0x3b() {
	// DEC SP
	Dec16(SP);
}
void Cpu::Inst0x3c() {
	// INC A
	Inc(A);
}
void Cpu::Inst0x3d() {
	// DEC A
	Dec(A);
}
void Cpu::Inst0x3e() {
	// LD A, d8
	A.Set(PopPC());
}
void Cpu::Inst0x3f() {
	// CCF
	SetC(!GetC());
	SetN(false);
	SetH(false);
}
void Cpu::Inst0x40() {
	// LD B, B
	B.Set(B.Get());
}
void Cpu::Inst0x41() {
	// LD B, C
	B.Set(C.Get());
}
void Cpu::Inst0x42() {
	// LD B, D
	B.Set(D.Get());
}
void Cpu::Inst0x43() {
	// LD B, E
	B.Set(E.Get());
}
void Cpu::Inst0x44() {
	// LD B, H
	B.Set(H.Get());
}
void Cpu::Inst0x45() {
	// LD B, L
	B.Set(L.Get());
}
void Cpu::Inst0x46() {
	// LD B, (HL)
	B.Set(mem->Read(HL.Get()));
}
void Cpu::Inst0x47() {
	// LD B, A
	B.Set(A.Get());
}
void Cpu::Inst0x48() {
	// LD C, B
	C.Set(B.Get());
}
void Cpu::Inst0x49() {
	// LD C, C
	C.Set(C.Get());
}
void Cpu::Inst0x4a() {
	// LD C, D
	C.Set(D.Get());
}
void Cpu::Inst0x4b() {
	// LD C, E
	C.Set(E.Get());
}
void Cpu::Inst0x4c() {
	// LD C, H
	C.Set(H.Get());
}
void Cpu::Inst0x4d() {
	// LD C, L
	C.Set(L.Get());
}
void Cpu::Inst0x4e() {
	// LD C, (HL)
	C.Set(mem->Read(HL.Get()));
}
void Cpu::Inst0x4f() {
	// LD C, A
	C.Set(A.Get());
}
void Cpu::Inst0x50() {
	// LD D, B
	D.Set(B.Get());
}
void Cpu::Inst0x51() {
	// LD D, C
	D.Set(C.Get());
}
void Cpu::Inst0x52() {
	// LD D, D
	D.Set(D.Get());
}
void Cpu::Inst0x53() {
	// LD D, E
	D.Set(E.Get());
}
void Cpu::Inst0x54() {
	// LD D, H
	D.Set(H.Get());
}
void Cpu::Inst0x55() {
	// LD D, L
	D.Set(L.Get());
}
void Cpu::Inst0x56() {
	// LD D, (HL)
	D.Set(mem->Read(HL.Get()));
}
void Cpu::Inst0x57() {
	// LD D, A
	D.Set(A.Get());
}
void Cpu::Inst0x58() {
	// LD E, B
	E.Set(B.Get());
}
void Cpu::Inst0x59() {
	// LD E, C
	E.Set(C.Get());
}
void Cpu::Inst0x5a() {
	// LD E, D
	E.Set(D.Get());
}
void Cpu::Inst0x5b() {
	// LD E, E
	E.Set(E.Get());
}
void Cpu::Inst0x5c() {
	// LD E, H
	E.Set(H.Get());
}
void Cpu::Inst0x5d() {
	// LD E, L
	E.Set(L.Get());
}
void Cpu::Inst0x5e() {
	// LD E, (HL)
	E.Set(mem->Read(HL.Get()));
}
void Cpu::Inst0x5f() {
	// LD E, A
	E.Set(A.Get());
}
void Cpu::Inst0x60() {
	// LD H, B
	H.Set(B.Get());
}
void Cpu::Inst0x61() {
	// LD H, C
	H.Set(C.Get());
}
void Cpu::Inst0x62() {
	// LD H, D
	H.Set(D.Get());
}
void Cpu::Inst0x63() {
	// LD H, E
	H.Set(E.Get());
}
void Cpu::Inst0x64() {
	// LD H, H
	H.Set(H.Get());
}
void Cpu::Inst0x65() {
	// LD H, L
	H.Set(L.Get());
}
void Cpu::Inst0x66() {
	// LD H, (HL)
	H.Set(mem->Read(HL.Get()));
}
void Cpu::Inst0x67() {
	// LD H, A
	H.Set(A.Get());
}
void Cpu::Inst0x68() {
	// LD L, B
	L.Set(B.Get());
}
void Cpu::Inst0x69() {
	// LD L, C
	L.Set(C.Get());
}
void Cpu::Inst0x6a() {
	// LD L, D
	L.Set(D.Get());
}
void Cpu::Inst0x6b() {
	// LD L, E
	L.Set(E.Get());
}
void Cpu::Inst0x6c() {
	// LD L, H
	L.Set(H.Get());
}
void Cpu::Inst0x6d() {
	// LD L, L
	L.Set(L.Get());
}
void Cpu::Inst0x6e() {
	// LD L, (HL)
	L.Set(mem->Read(HL.Get()));
}
void Cpu::Inst0x6f() {
	// LD L, A
	L.Set(A.Get());
}
void Cpu::Inst0x70() {
	// LD (HL), B
	mem->Write(HL.Get(), B.Get());
}
void Cpu::Inst0x71() {
	// LD (HL), C
	mem->Write(HL.Get(), C.Get());
}
void Cpu::Inst0x72() {
	// LD (HL), D
	mem->Write(HL.Get(), D.Get());
}
void Cpu::Inst0x73() {
	// LD (HL), E
	mem->Write(HL.Get(), E.Get());
}
void Cpu::Inst0x74() {
	// LD (HL), H
	mem->Write(HL.Get(), H.Get());
}
void Cpu::Inst0x75() {
	// LD (HL), L
	mem->Write(HL.Get(), L.Get());
}
void Cpu::Inst0x76() {
	// HALT
	Halt();
}
void Cpu::Inst0x77() {
	// LD (HL), A
	mem->Write(HL.Get(), A.Get());
}
void Cpu::Inst0x78() {
	// LD A, B
	A.Set(B.Get());
}
void Cpu::Inst0x79() {
	// LD A, C
	A.Set(C.Get());
}
void Cpu::Inst0x7a() {
	// LD A, D
	A.Set(D.Get());
}
void Cpu::Inst0x7b() {
	// LD A, E
	A.Set(E.Get());
}
void Cpu::Inst0x7c() {
	// LD A, H
	A.Set(H.Get());
}
void Cpu::Inst0x7d() {
	// LD A, L
	A.Set(L.Get());
}
void Cpu::Inst0x7e() {
	// LD A, (HL)
	A.Set(mem->Read(HL.Get()));
}
void Cpu::Inst0x7f() {
	// LD A, A
	A.Set(A.Get());
}
void Cpu::Inst0x80() {
	// ADD A,B
	Add(A, B.Get(), false);
}
void Cpu::Inst0x81() {
	// ADD A,C
	Add(A, C.Get(), false);
}
void Cpu::Inst0x82() {
	// ADD A,D
	Add(A, D.Get(), false);
}
void Cpu::Inst0x83() {
	// ADD A,E
	Add(A, E.Get(), false);
}
void Cpu::Inst0x84() {
	// ADD A,H
	Add(A, H.Get(), false);
}
void Cpu::Inst0x85() {
	// ADD A,L
	Add(A, L.Get(), false);
}
void Cpu::Inst0x86() {
	// ADD A,(HL)
	Add(A, mem->Read(HL.Get()), false);
}
void Cpu::Inst0x87() {
	// ADD A,A
	Add(A, A.Get(), false);
}
void Cpu::Inst0x88() {
	// ADC A,B
	Add(A, B.Get(), true);
}
void Cpu::Inst0x89() {
	// ADC A,C
	Add(A, C.Get(), true);
}
void Cpu::Inst0x8a() {
	// ADC A,D
	Add(A, D.Get(), true);
}
void Cpu::Inst0x8b() {
	// ADC A,E
	Add(A, E.Get(), true);
}
void Cpu::Inst0x8c() {
	// ADC A,H
	Add(A, H.Get(), true);
}
void Cpu::Inst0x8d() {
	// ADC A,L
	Add(A, L.Get(), true);
}
void Cpu::Inst0x8e() {
	// ADC A,(HL)
	Add(A, mem->Read(HL.Get()), true);
}
void Cpu::Inst0x8f() {
	// ADC A,A
	Add(A, A.Get(), true);
}
void Cpu::Inst0x90() {
	// SUB A, B
	Sub(A, B.Get(), false);
}
void Cpu::Inst0x91() {
	// SUB A, C
	Sub(A, C.Get(), false);
}
void Cpu::Inst0x92() {
	// SUB A, D
	Sub(A, D.Get(), false);
}
void Cpu::Inst0x93() {
	// SUB A, E
	Sub(A, E.Get(), false);
}
void Cpu::Inst0x94() {
	// SUB A, H
	Sub(A, H.Get(), false);
}
void Cpu::Inst0x95() {
	// SUB A, L
	Sub(A, L.Get(), false);
}
void Cpu::Inst0x96() {
	// SUB A, (HL)
	Sub(A, mem->Read(HL.Get()), false);
}
void Cpu::Inst0x97() {
	// SUB A, A
	Sub(A, A.Get(), false);
}
void Cpu::Inst0x98() {
	// SBC A,B
	Sub(A, B.Get(), true);
}
void Cpu::Inst0x99() {
	// SBC A,C
	Sub(A, C.Get(), true);
}
void Cpu::Inst0x9a() {
	// SBC A,D
	Sub(A, D.Get(), true);
}
void Cpu::Inst0x9b() {
	// SBC A,E
	Sub(A, E.Get(), true);
}
void Cpu::Inst0x9c() {
	// SBC A,H
	Sub(A, H.Get(), true);
}
void Cpu::Inst0x9d() {
	// SBC A,L
	Sub(A, L.Get(), true);
}
void Cpu::Inst0x9e() {
	// SBC A,(HL)
	Sub(A, mem->Read(HL.Get()), true);
}
void Cpu::Inst0x9f() {
	// SBC A,A
	Sub(A, A.Get(), true);
}
void Cpu::Inst0xa0() {
	// AND A, B
	And(A, B.Get());
}
void Cpu::Inst0xa1() {
	// AND A, C
	And(A, C.Get());
}
void Cpu::Inst0xa2() {
	// AND A, D
	And(A, D.Get());
}
void Cpu::Inst0xa3() {
	// AND A, E
	And(A, E.Get());
}
void Cpu::Inst0xa4() {
	// AND A, H
	And(A, H.Get());
}
void Cpu::Inst0xa5() {
	// AND A, L
	And(A, L.Get());
}
void Cpu::Inst0xa6() {
	// AND A, (HL)
	And(A, mem->Read(HL.Get()));
}
void Cpu::Inst0xa7() {
	// AND A, A
	And(A, A.Get());
}
void Cpu::Inst0xa8() {
	// XOR A, B
	Xor(A, B.Get());
}
void Cpu::Inst0xa9() {
	// XOR A, C
	Xor(A, C.Get());
}
void Cpu::Inst0xaa() {
	// XOR A, D
	Xor(A, D.Get());
}
void Cpu::Inst0xab() {
	// XOR A, E
	Xor(A, E.Get());
}
void Cpu::Inst0xac() {
	// XOR A, H
	Xor(A, H.Get());
}
void Cpu::Inst0xad() {
	// XOR A, L
	Xor(A, L.Get());
}
void Cpu::Inst0xae() {
	// XOR A, (HL)
	Xor(A, mem->Read(HL.Get()));
}
void Cpu::Inst0xaf() {
	// XOR A, A
	Xor(A, A.Get());
}
void Cpu::Inst0xb0() {
	// OR B
	Or(A, B.Get());
}
void Cpu::Inst0xb1() {
	// OR C
	Or(A, C.Get());
}
void Cpu::Inst0xb2() {
	// OR D
	Or(A, D.Get());
}
void Cpu::Inst0xb3() {
	// OR E
	Or(A, E.Get());
}
void Cpu::Inst0xb4() {
	// OR H
	Or(A, H.Get());
}
void Cpu::Inst0xb5() {
	// OR L
	Or(A, L.Get());
}
void Cpu::Inst0xb6() {
	// OR (HL)
	Or(A, mem->Read(HL.Get()));
}
void Cpu::Inst0xb7() {
	// OR A
	Or(A, A.Get());
}
void Cpu::Inst0xb8() {
	// CP B
	Cp(A, B.Get());
}
void Cpu::Inst0xb9() {
	// CP C
	Cp(A, C.Get());
}
void Cpu::Inst0xba() {
	// CP D
	Cp(A, D.Get());
}
void Cpu::Inst0xbb() {
	// CP E
	Cp(A, E.Get());
}
void Cpu::Inst0xbc() {
	// CP H
	Cp(A, H.Get());
}
void Cpu::Inst0xbd() {
	// CP L
	Cp(A, L.Get());
}
void Cpu::Inst0xbe() {
	// CP (HL)
	Cp(A, mem->Read(HL.Get()));
}
void Cpu::Inst0xbf() {
	// CP A
	Cp(A, A.Get());
}
void Cpu::Inst0xc0() {
	// RET NZ
	if (!GetZ()) {
		Ret();
		additionnalTicks += 12;
	}
}
void Cpu::Inst0xc1() {
	// POP BC
	BC.Set(PopStack());
}
void Cpu::Inst0xc2() {
	// JP NZ, a16
	uint16 jump = PopPC16();
	if (!GetZ()) {
		PC = jump;
		additionnalTicks += 4;
	}
}
void Cpu::Inst0xc3() {
	// JP a16
	PC = PopPC16();
}
void Cpu::Inst0xc4() {
	// CALL NZ
	uint16 jump = PopPC16();
	if (!GetZ()) {
		Call(jump);
		additionnalTicks += 12;
	}
}
void Cpu::Inst0xc5() {
	// PUSH BC
	PushStack(BC.Get());
}
void Cpu::Inst0xc6() {
	// ADD A, d8
	Add(A, PopPC(), false);
}
void Cpu::Inst0xc7() {
	// RST 0x00
	Call(0x0000);
}
void Cpu::Inst0xc8() {
	// RET Z
	if (GetZ()) {
		Ret();
		additionnalTicks += 12;
	}
}
void Cpu::Inst0xc9() {
	// RET
	Ret();
}
void Cpu::Inst0xca() {
	// JP Z, a16
	uint16 jump = PopPC16();
	if (GetZ()) {
		PC = jump;
		additionnalTicks += 4;
	}
}
void Cpu::Inst0xcb() {
	// PREFIX CB
	additionnalTicks += ExecuteCBOPCode(this, PopPC());
}
void Cpu::Inst0xcc() {
	// CALL Z
	uint16 jump = PopPC16();
	if (GetZ()) {
		Call(jump);
		additionnalTicks += 12;
	}
}
void Cpu::Inst0xcd() {
	// CALL a16
	Call(PopPC16());
}
void Cpu::Inst0xce() {
	// ADC A, d8
	Add(A, PopPC(), true);
}
void Cpu::Inst0xcf() {
	// RST 0x08
	Call(0x0008);
}
void Cpu::Inst0xd0() {
	// RET NC
	if (!GetC()) {
		Ret();
		additionnalTicks += 12;
	}
}
void Cpu::Inst0xd1() {
	// POP DE
	DE.Set(PopStack());
}
void Cpu::Inst0xd2() {
	// JP NC, a16
	uint16 jump = PopPC16();
	if (!GetC()) {
		PC = jump;
		additionnalTicks += 4;
	}
}
void Cpu::Inst0xd3() {
	INVALID_OP;
}
void Cpu::Inst0xd4() {
	// CALL NC
	uint16 jump = PopPC16();
	if (!GetC()) {
		Call(jump);
		additionnalTicks += 12;
	}
}
void Cpu::Inst0xd5() {
	// PUSH DE
	PushStack(DE.Get());
}
void Cpu::Inst0xd6() {
	// SUB A, d8
	Sub(A, PopPC(), false);
}
void Cpu::Inst0xd7() {
	// RST 0x10
	Call(0x0010);
}
void Cpu::Inst0xd8() {
	// RET C
	if (GetC()) {
		Ret();
		additionnalTicks += 12;
	}
}
void Cpu::Inst0xd9() {
	// RETI
	Ret();
	interuptsEnabled = true;
}
void Cpu::Inst0xda() {
	// JP C, a16
	uint16 jump = PopPC16();
	if (GetC()) {
		PC = jump;
		additionnalTicks += 4;
	}
}
void Cpu::Inst0xdb() {
	INVALID_OP;
}
void Cpu::Inst0xdc() {
	// CALL C
	uint16 jump = PopPC16();
	if (GetC()) {
		Call(jump);
		additionnalTicks += 12;
	}
}
void Cpu::Inst0xdd() {
	INVALID_OP;
}
void Cpu::Inst0xde() {
	// SBC A, d8
	Sub(A, PopPC(), true);
}
void Cpu::Inst0xdf() {
	// RST 0x18
	Call(0x0018);
}
void Cpu::Inst0xe0() {
	// LDH (a8), A
	mem->Write(0xFF00 + PopPC(), A.Get());
}
void Cpu::Inst0xe1() {
	// POP HL
	HL.Set(PopStack());
}
void Cpu::Inst0xe2() {
	// LD (C), A
	mem->Write(0xFF00 + C.Get(), A.Get());
}
void Cpu::Inst0xe3() {
	INVALID_OP;
}
void Cpu::Inst0xe4() {
	INVALID_OP;
}
void Cpu::Inst0xe5() {
	// PUSH HL
	PushStack(HL.Get());
}
void Cpu::Inst0xe6() {
	// AND A, d8
	And(A, PopPC());
}
void Cpu::Inst0xe7() {
	// RST 0x20
	Call(0x0020);
}
void Cpu::Inst0xe8() {
	// ADD SP, r8
	Add16Signed(SP, PopPC());
	SetZ(false);
}
void Cpu::Inst0xe9() {
	// JP HL
	PC = HL.Get();
}
void Cpu::Inst0xea() {
	// LD (a16), A
	mem->Write(PopPC16(), A.Get());
}
void Cpu::Inst0xeb() {
	INVALID_OP;
}
void Cpu::Inst0xec() {
	INVALID_OP;
}
void Cpu::Inst0xed() {
	INVALID_OP;
}
void Cpu::Inst0xee() {
	// XOR A, d8
	Xor(A, PopPC());
}
void Cpu::Inst0xef() {
	// RST 0x28
	Call(0x0028);
}
void Cpu::Inst0xf0() {
	// LDH A, (a8)
	A.Set(mem->Read(0xFF00 + PopPC()));
}
void Cpu::Inst0xf1() {
	// TODO: Apparently this might affect flags? But I have no idea how
	// POP AF
	AF.Set(PopStack());
}
void Cpu::Inst0xf2() {
	// LD A, (C)
	A.Set(mem->Read(0xFF00 + C.Get()));
}
void Cpu::Inst0xf3() {
	// DI
	interuptsOn = false;
}
void Cpu::Inst0xf4() {
	INVALID_OP;
}
void Cpu::Inst0xf5() {
	// PUSH AF
	PushStack(AF.Get());
}
void Cpu::Inst0xf6() {
	// OR A, d8
	Or(A, PopPC());
}
void Cpu::Inst0xf7() {
	// RST 0x30
	Call(0x0030);
}
void Cpu::Inst0xf8() {
	// LD HL, SP+r8
	HL.Set(SP.Get());
	Add16Signed(HL, PopPC());
}
void Cpu::Inst0xf9() {
	// LD SP, HL
	SP.Set(HL.Get());
}
void Cpu::Inst0xfa() {
	// LD A, (a16)
	A.Set(mem->Read(PopPC16()));
}
void Cpu::Inst0xfb() {
	// EI
	interuptsEnabled = true;
}
void Cpu::Inst0xfc() {
	INVALID_OP;
}
void Cpu::Inst0xfd() {
	INVALID_OP;
}
void Cpu::Inst0xfe() {
	// CP A, d8
	Cp(A, PopPC());
}
void Cpu::Inst0xff() {
	// RST 0x38
	Call(0x0038);
}

Cpu::InstructionPtr Cpu::s_instructions[0x100] = {
	&Cpu::Inst0x00, &Cpu::Inst0x01, &Cpu::Inst0x02, &Cpu::Inst0x03, &Cpu::Inst0x04, &Cpu::Inst0x05, &Cpu::Inst0x06, &Cpu::Inst0x07, &Cpu::Inst0x08,
	&Cpu::Inst0x09, &Cpu::Inst0x0a, &Cpu::Inst0x0b, &Cpu::Inst0x0c, &Cpu::Inst0x0d, &Cpu::Inst0x0e, &Cpu::Inst0x0f, &Cpu::Inst0x10, &Cpu::Inst0x11,
	&Cpu::Inst0x12, &Cpu::Inst0x13, &Cpu::Inst0x14, &Cpu::Inst0x15, &Cpu::Inst0x16, &Cpu::Inst0x17, &Cpu::Inst0x18, &Cpu::Inst0x19, &Cpu::Inst0x1a,
	&Cpu::Inst0x1b, &Cpu::Inst0x1c, &Cpu::Inst0x1d, &Cpu::Inst0x1e, &Cpu::Inst0x1f, &Cpu::Inst0x20, &Cpu::Inst0x21, &Cpu::Inst0x22, &Cpu::Inst0x23,
	&Cpu::Inst0x24, &Cpu::Inst0x25, &Cpu::Inst0x26, &Cpu::Inst0x27, &Cpu::Inst0x28, &Cpu::Inst0x29, &Cpu::Inst0x2a, &Cpu::Inst0x2b, &Cpu::Inst0x2c,
	&Cpu::Inst0x2d, &Cpu::Inst0x2e, &Cpu::Inst0x2f, &Cpu::Inst0x30, &Cpu::Inst0x31, &Cpu::Inst0x32, &Cpu::Inst0x33, &Cpu::Inst0x34, &Cpu::Inst0x35,
	&Cpu::Inst0x36, &Cpu::Inst0x37, &Cpu::Inst0x38, &Cpu::Inst0x39, &Cpu::Inst0x3a, &Cpu::Inst0x3b, &Cpu::Inst0x3c, &Cpu::Inst0x3d, &Cpu::Inst0x3e,
	&Cpu::Inst0x3f, &Cpu::Inst0x40, &Cpu::Inst0x41, &Cpu::Inst0x42, &Cpu::Inst0x43, &Cpu::Inst0x44, &Cpu::Inst0x45, &Cpu::Inst0x46, &Cpu::Inst0x47,
	&Cpu::Inst0x48, &Cpu::Inst0x49, &Cpu::Inst0x4a, &Cpu::Inst0x4b, &Cpu::Inst0x4c, &Cpu::Inst0x4d, &Cpu::Inst0x4e, &Cpu::Inst0x4f, &Cpu::Inst0x50,
	&Cpu::Inst0x51, &Cpu::Inst0x52, &Cpu::Inst0x53, &Cpu::Inst0x54, &Cpu::Inst0x55, &Cpu::Inst0x56, &Cpu::Inst0x57, &Cpu::Inst0x58, &Cpu::Inst0x59,
	&Cpu::Inst0x5a, &Cpu::Inst0x5b, &Cpu::Inst0x5c, &Cpu::Inst0x5d, &Cpu::Inst0x5e, &Cpu::Inst0x5f, &Cpu::Inst0x60, &Cpu::Inst0x61, &Cpu::Inst0x62,
	&Cpu::Inst0x63, &Cpu::Inst0x64, &Cpu::Inst0x65, &Cpu::Inst0x66, &Cpu::Inst0x67, &Cpu::Inst0x68, &Cpu::Inst0x69, &Cpu::Inst0x6a, &Cpu::Inst0x6b,
	&Cpu::Inst0x6c, &Cpu::Inst0x6d, &Cpu::Inst0x6e, &Cpu::Inst0x6f, &Cpu::Inst0x70, &Cpu::Inst0x71, &Cpu::Inst0x72, &Cpu::Inst0x73, &Cpu::Inst0x74,
	&Cpu::Inst0x75, &Cpu::Inst0x76, &Cpu::Inst0x77, &Cpu::Inst0x78, &Cpu::Inst0x79, &Cpu::Inst0x7a, &Cpu::Inst0x7b, &Cpu::Inst0x7c, &Cpu::Inst0x7d,
	&Cpu::Inst0x7e, &Cpu::Inst0x7f, &Cpu::Inst0x80, &Cpu::Inst0x81, &Cpu::Inst0x82, &Cpu::Inst0x83, &Cpu::Inst0x84, &Cpu::Inst0x85, &Cpu::Inst0x86,
	&Cpu::Inst0x87, &Cpu::Inst0x88, &Cpu::Inst0x89, &Cpu::Inst0x8a, &Cpu::Inst0x8b, &Cpu::Inst0x8c, &Cpu::Inst0x8d, &Cpu::Inst0x8e, &Cpu::Inst0x8f,
	&Cpu::Inst0x90, &Cpu::Inst0x91, &Cpu::Inst0x92, &Cpu::Inst0x93, &Cpu::Inst0x94, &Cpu::Inst0x95, &Cpu::Inst0x96, &Cpu::Inst0x97, &Cpu::Inst0x98,
	&Cpu::Inst0x99, &Cpu::Inst0x9a, &Cpu::Inst0x9b, &Cpu::Inst0x9c, &Cpu::Inst0x9d, &Cpu::Inst0x9e, &Cpu::Inst0x9f, &Cpu::Inst0xa0, &Cpu::Inst0xa1,
	&Cpu::Inst0xa2, &Cpu::Inst0xa3, &Cpu::Inst0xa4, &Cpu::Inst0xa5, &Cpu::Inst0xa6, &Cpu::Inst0xa7, &Cpu::Inst0xa8, &Cpu::Inst0xa9, &Cpu::Inst0xaa,
	&Cpu::Inst0xab, &Cpu::Inst0xac, &Cpu::Inst0xad, &Cpu::Inst0xae, &Cpu::Inst0xaf, &Cpu::Inst0xb0, &Cpu::Inst0xb1, &Cpu::Inst0xb2, &Cpu::Inst0xb3,
	&Cpu::Inst0xb4, &Cpu::Inst0xb5, &Cpu::Inst0xb6, &Cpu::Inst0xb7, &Cpu::Inst0xb8, &Cpu::Inst0xb9, &Cpu::Inst0xba, &Cpu::Inst0xbb, &Cpu::Inst0xbc,
	&Cpu::Inst0xbd, &Cpu::Inst0xbe, &Cpu::Inst0xbf, &Cpu::Inst0xc0, &Cpu::Inst0xc1, &Cpu::Inst0xc2, &Cpu::Inst0xc3, &Cpu::Inst0xc4, &Cpu::Inst0xc5,
	&Cpu::Inst0xc6, &Cpu::Inst0xc7, &Cpu::Inst0xc8, &Cpu::Inst0xc9, &Cpu::Inst0xca, &Cpu::Inst0xcb, &Cpu::Inst0xcc, &Cpu::Inst0xcd, &Cpu::Inst0xce,
	&Cpu::Inst0xcf, &Cpu::Inst0xd0, &Cpu::Inst0xd1, &Cpu::Inst0xd2, &Cpu::Inst0xd3, &Cpu::Inst0xd4, &Cpu::Inst0xd5, &Cpu::Inst0xd6, &Cpu::Inst0xd7,
	&Cpu::Inst0xd8, &Cpu::Inst0xd9, &Cpu::Inst0xda, &Cpu::Inst0xdb, &Cpu::Inst0xdc, &Cpu::Inst0xdd, &Cpu::Inst0xde, &Cpu::Inst0xdf, &Cpu::Inst0xe0,
	&Cpu::Inst0xe1, &Cpu::Inst0xe2, &Cpu::Inst0xe3, &Cpu::Inst0xe4, &Cpu::Inst0xe5, &Cpu::Inst0xe6, &Cpu::Inst0xe7, &Cpu::Inst0xe8, &Cpu::Inst0xe9,
	&Cpu::Inst0xea, &Cpu::Inst0xeb, &Cpu::Inst0xec, &Cpu::Inst0xed, &Cpu::Inst0xee, &Cpu::Inst0xef, &Cpu::Inst0xf0, &Cpu::Inst0xf1, &Cpu::Inst0xf2,
	&Cpu::Inst0xf3, &Cpu::Inst0xf4, &Cpu::Inst0xf5, &Cpu::Inst0xf6, &Cpu::Inst0xf7, &Cpu::Inst0xf8, &Cpu::Inst0xf9, &Cpu::Inst0xfa, &Cpu::Inst0xfb,
	&Cpu::Inst0xfc, &Cpu::Inst0xfd, &Cpu::Inst0xfe, &Cpu::Inst0xff
};

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
