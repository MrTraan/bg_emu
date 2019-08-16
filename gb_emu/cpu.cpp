#include "cpu.h"
#include "memory.h"

void Cpu::Add(Register8 & reg, byte val, bool useCarry) {
	byte valReg = reg.Get();
	byte carry = GetC() && useCarry ? 1 : 0;
	int16 total = valReg + val + carry;
	reg.Set((byte)total);

	SetZ(total == 0);
	SetN(false);
	SetH((val & 0xF) + (valReg & 0xF) + carry > 0xF);
	SetC(total > 0xFF);
}

void Cpu::Sub(Register8 & reg, byte val, bool useCarry) {
	byte valReg = reg.Get();
	byte carry = GetC() && useCarry ? 1 : 0;
	int16 total = valReg - val - carry;
	reg.Set((byte)total);

	SetZ(total == 0);
	SetN(true);
	SetH((int16)(val & 0xF) - (valReg & 0xF) - carry < 0);
	SetC(total < 0);
}

void Cpu::And(Register8 & reg, byte val) {
	byte valReg = reg.Get();
	byte total = valReg & val;
	reg.Set((byte)total);

	SetZ(total == 0);
	SetN(false);
	SetH(true);
	SetC(false);
}

void Cpu::Or(Register8 & reg, byte val) {
	byte valReg = reg.Get();
	byte total = valReg | val;
	reg.Set((byte)total);

	SetZ(total == 0);
	SetN(false);
	SetH(false);
	SetC(false);
}

void Cpu::Xor(Register8 & reg, byte val) {
	byte valReg = reg.Get();
	byte total = valReg ^ val;
	reg.Set((byte)total);

	SetZ(total == 0);
	SetN(false);
	SetH(false);
	SetC(false);
}

void Cpu::Cp(Register8 & reg, byte val) {
	byte valReg = reg.Get();
	byte total = valReg - val;
	SetZ(total == 0);
	SetN(true);
	SetH((valReg & 0x0f) > (val & 0x0f));
	SetC(valReg > val);
}

void Cpu::Inc(Register8 & reg) {
	byte valReg = reg.Get();
	byte total = valReg + 1;
	SetZ(total == 0);
	SetN(false);
	SetH((valReg & 0x0f) + 1 > 0x0f);
}

void Cpu::Dec(Register8 & reg) {
	byte valReg = reg.Get();
	byte total = valReg - 1;
	SetZ(total == 0);
	SetN(false);
	SetH((valReg & 0x0f) > 0x0f);
}

void Cpu::Add16(Register16 & reg, uint16 val) {
	uint16 valReg = reg.Get();
	int total = valReg + val;
	reg.Set((uint16)total);
	SetN(false);
	SetH((valReg & 0xfff) > (total & 0xfff));
	SetC(total > 0xffff);
}

void Cpu::Add16Signed(Register16 & reg, byte val) {
	uint16 valReg = reg.Get();
	int total = valReg + val;
	reg.Set((uint16)total);
	SetZ(false);
	SetN(false);
	SetH(((valReg ^ val ^ total) & 0x10) == 0x10);
	SetC(((valReg ^ val ^ total) & 0x100) == 0x100);
}

void Cpu::Inc16(Register16 & reg) {
	reg.Set(reg.Get() + 1);
}

void Cpu::Dec16(Register16 & reg) {
	reg.Set(reg.Get() - 1);
}

byte PopPC(Cpu * cpu, Memory * mem) {
	byte opcode = mem->Read(cpu->PC);
	cpu->PC++;
	return opcode;
}

uint16 PopPC16(Cpu * cpu, Memory * mem) {
	byte val1 = mem->Read(cpu->PC);
	cpu->PC++;
	byte val2 = mem->Read(cpu->PC);
	cpu->PC++;
	return ((uint16)val2 << 8) | val1;
}

// Return Cycles used
int ExecuteNextOPCode(Cpu * cpu, Memory * mem) {
	constexpr bool HIGH = true;
	constexpr bool LOW = false;
	byte opcode = PopPC(cpu, mem);
	int ticksUsed = 0;

	// All instructions are detailled here : https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html

	switch (opcode) {
	case 0x0: {
		// NOP
		ticksUsed = 4;
		break;
	}

	case 0x01: {
		// LD BC, d16
		uint16 val = PopPC16(cpu, mem);
		cpu->BC.Set(val);
		ticksUsed = 12;
		break;
	}

	case 0x02: {
		// LD (BC), A
		uint16 addr = cpu->BC.Get();
		byte val = cpu->A().Get();
		mem->Write(addr, val);
		ticksUsed = 8;
		break;
	}

	case 0x03: {
		// INC BC
		cpu->Inc16(cpu->BC);
		ticksUsed = 8;
		break;
	}

	case 0x04: {
		// INC B
		cpu->Inc(cpu->B());
		ticksUsed = 4;
		break;
	}
	
	case 0x05: {
		// DEC B
		cpu->Dec(cpu->B());
		ticksUsed = 4;
		break;
	}

	case 0x06: {
		// LD B, d8
		byte val = PopPC(cpu, mem);
		cpu->B().Set(val);
		ticksUsed = 8;
		break;
	}

	case 0x07: {
		// RLCA
		byte val = cpu->A().Get();
		byte res = (val << 1) || (val >> 7); // Put last bit first
		cpu->A().Set(res);
		cpu->SetZ(false);
		cpu->SetN(false);
		cpu->SetH(false);
		cpu->SetC(val > 0x7f);
		ticksUsed = 4;
		break;
	}

	case 0x08: {
		// LD (a16), SP
		uint16 addr = PopPC16(cpu, mem);
		mem->Write(addr, cpu->SP.low.Get());
		mem->Write(addr + 1, cpu->SP.high.Get());
		ticksUsed = 20;
		break;
	}

	case 0x09: {
		// ADD HL, BC
		cpu->Add16(cpu->HL, cpu->BC.Get());
		ticksUsed = 8;
		break;
	}

	case 0x0a: {
		// LD A, (BC)
		byte val = mem->Read(cpu->BC.Get());
		cpu->A().Set(val);
		ticksUsed = 8;
		break;
	}

	case 0x0b: {
		// DEC BC
		cpu->Dec16(cpu->BC);
		ticksUsed = 8;
		break;
	}

	case 0x0c: {
		// INC C
		cpu->Inc(cpu->C());
		ticksUsed = 4;
		break;
	}

	case 0x0d: {
		// DEC C
		cpu->Dec(cpu->C());
		ticksUsed = 4;
		break;
	}

	case 0x0e: {
		// LD C, d8
		byte val = PopPC(cpu, mem);
		cpu->C().Set(val);
		ticksUsed = 8;
		break;
	}

	case 0x0f: {
		// RRCA
		byte val = cpu->A().Get();
		byte res = (val >> 1) | ((val & 1) << 7);
		cpu->A().Set(res);
		cpu->SetZ(false);
		cpu->SetN(false);
		cpu->SetH(false);
		cpu->SetC(res > 0x7f);
		ticksUsed = 4;
		break;
	}

	default: {
		// Unimplemented instruction
		DEBUG_BREAK;
	}
	}
	
	return ticksUsed; // Should return ticks needed
}

