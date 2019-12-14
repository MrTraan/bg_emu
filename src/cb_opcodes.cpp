#include "cb_opcodes.h"
#include "cpu.h"
#include "memory.h"

static int CBopcodeCyclesCost[0x100] = {
	//  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
		2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, // 0
		2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, // 1
		2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, // 2
		2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, // 3
		2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2, // 4
		2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2, // 5
		2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2, // 6
		2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2, // 7
		2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, // 8
		2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, // 9
		2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, // A
		2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, // B
		2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, // C
		2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, // D
		2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, // E
		2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, // F
};

byte Rlc(Cpu* cpu, byte val) {
	byte carry = val >> 7;
	byte rot = (val << 1) & 0xff | carry;

	cpu->SetZ(rot == 0);
	cpu->SetN(false);
	cpu->SetH(false);
	cpu->SetC(carry == 1);
	return rot;
}

byte Rl(Cpu* cpu, byte val) {
	byte carry = val >> 7;
	byte rot = (val << 1) & 0xff | (cpu->GetC() ? 1 : 0);

	cpu->SetZ(rot == 0);
	cpu->SetN(false);
	cpu->SetH(false);
	cpu->SetC(carry == 1);
	return rot;
}

byte Rrc(Cpu* cpu, byte val) {
	byte carry = val & 1;
	byte rot = (val >> 1) | (carry << 7);

	cpu->SetZ(rot == 0);
	cpu->SetN(false);
	cpu->SetH(false);
	cpu->SetC(carry == 1);
	return rot;
}

byte Rr(Cpu* cpu, byte val) {
	byte carry = val & 1;
	byte rot = (val >> 1) | ((cpu->GetC() ? 1 : 0) << 7);

	cpu->SetZ(rot == 0);
	cpu->SetN(false);
	cpu->SetH(false);
	cpu->SetC(carry == 1);
	return rot;
}

byte Sla(Cpu* cpu, byte val) {
	byte carry = val >> 7;
	byte rot = (val << 1) & 0xff;

	cpu->SetZ(rot == 0);
	cpu->SetN(false);
	cpu->SetH(false);
	cpu->SetC(carry == 1);
	return rot;
}

byte Sra(Cpu* cpu, byte val) {
	byte rot = (val & 0x80) | (val >> 1);

	cpu->SetZ(rot == 0);
	cpu->SetN(false);
	cpu->SetH(false);
	cpu->SetC((val & 1) == 1);
	return rot;
}

byte Srl(Cpu* cpu, byte val) {
	byte rot = val >> 1;

	cpu->SetZ(rot == 0);
	cpu->SetN(false);
	cpu->SetH(false);
	cpu->SetC((val & 1) == 1);
	return rot;
}

byte Swap(Cpu* cpu, byte val) {
	byte swapped = ((val << 4) & 0xf0) | (val >> 4);
	cpu->SetZ(swapped == 0);
	cpu->SetN(false);
	cpu->SetH(false);
	cpu->SetC(false);
	return swapped;
}

void Bit(Cpu* cpu, byte val, byte bit) {
	cpu->SetZ(((val >> bit) & 1) == 0);
	cpu->SetN(false);
	cpu->SetH(true);
}

int ExecuteCBOPCode(Cpu* cpu, uint16 opcode) {
	uint16 opBase = opcode - (opcode % 8);
	uint16 opOffset = opcode % 8;

	byte input = 0;
	if (opOffset == 0) {
		input = cpu->BC.high.Get();
	}
	else if (opOffset == 1) {
		input = cpu->BC.low.Get();
	}
	else if (opOffset == 2) {
		input = cpu->DE.high.Get();
	}
	else if (opOffset == 3) {
		input = cpu->DE.low.Get();
	}
	else if (opOffset == 4) {
		input = cpu->HL.high.Get();
	}
	else if (opOffset == 5) {
		input = cpu->HL.low.Get();
	}
	else if (opOffset == 6) {
		input = cpu->mem->Read(cpu->HL.Get());
	}
	else if (opOffset == 7) {
		input = cpu->A.Get();
	}

	byte output = 0;
	bool hasOutput = true;
	if (opBase == 0x00) {
		output = Rlc(cpu, input);
	}
	else if (opBase == 0x08) {
		output = Rrc(cpu, input);
	}
	else if (opBase == 0x10) {
		output = Rl(cpu, input);
	}
	else if (opBase == 0x18) {
		output = Rr(cpu, input);
	}
	else if (opBase == 0x20) {
		output = Sla(cpu, input);
	}
	else if (opBase == 0x28) {
		output = Sra(cpu, input);
	}
	else if (opBase == 0x30) {
		output = Swap(cpu, input);
	}
	else if (opBase == 0x38) {
		output = Srl(cpu, input);
	}
	else if (opBase >= 0x40 && opBase < 0x80) {
		// BIT instruction
		byte bitValue = (opBase - 0x40) / 8;
		Bit(cpu, input, bitValue);
		hasOutput = false;
	}
	else if (opBase >= 0x80 && opBase < 0xc0) {
		// RES instruction
		byte bitValue = (opBase - 0x80) / 8;
		output = BIT_UNSET(input, bitValue);
	}
	else if (opBase >= 0xc0) {
		// RES instruction
		byte bitValue = (opBase - 0xc0) / 8;
		output = BIT_SET(input, bitValue);
	}

	if (hasOutput) {
		if (opOffset == 0) {
			cpu->BC.high.Set(output);
		}
		else if (opOffset == 1) {
			cpu->BC.low.Set(output);
		}
		else if (opOffset == 2) {
			cpu->DE.high.Set(output);
		}
		else if (opOffset == 3) {
			cpu->DE.low.Set(output);
		}
		else if (opOffset == 4) {
			cpu->HL.high.Set(output);
		}
		else if (opOffset == 5) {
			cpu->HL.low.Set(output);
		}
		else if (opOffset == 6) {
			cpu->mem->Write(cpu->HL.Get(), output);
		}
		else if (opOffset == 7) {
			cpu->A.Set(output);
		}
	}

	return CBopcodeCyclesCost[opcode];
}
