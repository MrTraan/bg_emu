#include <stdio.h>
#include "cpu.h"
#include "cb_opcodes.h"
#include "gameboy.h"

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
int Cpu::ExecuteNextOPCode( Gameboy * gb) {
	// All instructions are detailled here : https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html
	byte opcode = PopPC(gb);
	int  ticksUsed = opcodeCyclesCost[opcode] * 4;
	additionnalTicks = 0;
	lastInstructionName = s_instructionsNames[opcode];
	//printf("%s\n", s_instructionsNames[opcode]);
	//static bool startLogging = false;
	//if ( PC - 1 == 0x100 ) {
	//	startLogging = true;
	//}
	//if (startLogging)
		//printf("0x%02x 0x%04x\n", opcode, PC - 1);

	ExecuteInstruction( opcode, gb );
	return ticksUsed + additionnalTicks;
}

void Cpu::UpdateTimer(int clock,  Gameboy * gb) {
	divider += clock;
	if (divider >= 255) {
		divider -= 255;
		byte div = gb->Read(DIV);
		if (div == 255)
			gb->mem.highRAM[DIV - 0xFF00] = 0;
		else
			gb->mem.highRAM[DIV - 0xFF00]++;
	}

	byte tac = gb->mem.highRAM[TAC - 0xFF00];
	byte frequency = tac & 3;
	constexpr int threshold[4] = {1024, 16, 64, 256};

	if (!BIT_IS_SET(tac, 2))
		return;

	clockCounter += clock;
	if (clockCounter > threshold[frequency])
	{
		clockCounter -= threshold[frequency];
		byte tima = gb->mem.highRAM[TIMA - 0xFF00];
		if (tima == 0xFF)
		{
			gb->mem.highRAM[TIMA - 0xFF00] = gb->mem.highRAM[TMA-0xFF00];
			gb->RaiseInterupt(2);
		}
		else
		{
			gb->mem.highRAM[TIMA - 0xFF00]++;
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
void Cpu::Call(uint16 addr,  Gameboy * gb) {
	PushStack(PC, gb);
	PC = addr;
}

// Returns by setting PC value to the value on the stack
void Cpu::Ret( Gameboy * gb) {
	PC = PopStack(gb);
}

byte Cpu::PopPC( Gameboy * gb) {
	byte opcode = gb->Read(PC);
	PC++;
	return opcode;
}

uint16 Cpu::PopPC16( Gameboy * gb) {
	byte val1 = gb->Read(PC);
	PC++;
	byte val2 = gb->Read(PC);
	PC++;
	return ((uint16)val2 << 8) | val1;
}

// Pushes a 16 bit value on the stack, and decrement the stack pointer
void Cpu::PushStack(uint16 val,  Gameboy * gb) {
	gb->Write(SP.Get() - 1, BIT_HIGH_8(val));
	gb->Write(SP.Get() - 2, BIT_LOW_8(val));
	SP.Set(SP.Get() - 2);
}

// Read the 16 bits on the stack, and increment the stack pointer
uint16 Cpu::PopStack( Gameboy * gb) {
	byte b1 = gb->Read(SP.Get());
	byte b2 = gb->Read(SP.Get() + 1);
	SP.Set(SP.Get() + 2);
	return ((uint16)b2 << 8) | b1;
}

void Cpu::Halt() {
	isOnHalt = true;
}

static uint16 interruptAddresses[] = {
	0x40, // V-Blank
	0x48, // LCDC Status
	0x50, // Timer Overflow
	0x58, // Serial Transfer
	0x60, // Hi-Lo P10-P13
};

int Cpu::ProcessInterupts( Gameboy * gb) {
	if (interuptsEnabled) {
		interuptsOn = true;
		interuptsEnabled = false;
		return 0;
	}
	if (!interuptsOn && !isOnHalt) {
		return 0;
	}

	byte mask = gb->Read(0xff0f);
	byte enabledMask = gb->Read(0xffff);

	for (byte i = 0; i < 5; i++) {
		if (BIT_IS_SET(mask, i) && BIT_IS_SET(enabledMask, i)) {
			if (!interuptsOn && isOnHalt) {
				isOnHalt = false;
			} else {
				interuptsOn = false;
				isOnHalt = false;
				mask = BIT_UNSET(mask, i);
				gb->Write(0xff0f, mask);

				PushStack(PC, gb);
				PC = interruptAddresses[i];
			}
			return 20;
		}
	}

	return 0;
}
