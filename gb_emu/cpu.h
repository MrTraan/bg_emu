#pragma once

#include "gb_emu.h"

struct Memory;

struct Register8 {
	byte mask = 0xFF;

	byte Get() { return value; }
	void Set(uint8 newVal) {
		value = newVal;
		ApplyMask();
	}

	void ApplyMask() { value &= mask; }

private:
	byte value = 0x0;
};

struct Register16 {
	Register8 high;
	Register8 low;

	void Set(uint16 val) {
		high.Set(val >> 8);
		low.Set(val & 0xFF);
	}

	uint16 Get() { return ((uint16)high.Get() << 8) + low.Get(); }
};

struct Cpu {
	typedef void (Cpu::* InstructionPtr)(void);
	static InstructionPtr s_instructions[0x100];

	Register16	AF;
	Register16  BC;
	Register16  DE;
	Register16  HL;
	Register8& A = AF.high;
	Register8& F = AF.low;
	Register8& B = BC.high;
	Register8& C = BC.low;
	Register8& D = DE.high;
	Register8& E = DE.low;
	Register8& H = HL.high;
	Register8& L = HL.low;

	uint16	   PC;
	Register16 SP;

	Memory* mem;

	int additionnalTicks = 0;

	bool interuptsEnabled = true;
	bool interuptsOn = true;

	// int divider

	// Initialize CPU with default values
	Cpu(Memory* _mem) : mem(_mem) {
		PC = 0x100;
		AF.Set(0x01B0);
		BC.Set(0);
		DE.Set(0xFF56);
		HL.Set(0x000D);
		SP.Set(0xFFFE);

		F.mask = 0xF0;
	}

	int	   ExecuteNextOPCode();
	int    ExecuteCBOPCode(uint16 opcode, byte arg);
	byte   PopPC();
	uint16 PopPC16();
	void   PushStack(uint16 val);
	uint16 PopStack();

	void Add(Register8& reg, byte val, bool useCarry);
	void Sub(Register8& reg, byte val, bool useCarry);
	void And(Register8& reg, byte val);
	void Or(Register8& reg, byte val);
	void Xor(Register8& reg, byte val);
	void Cp(Register8& reg, byte val);
	void Inc(Register8& reg);
	void Dec(Register8& reg);
	void Call(uint16 addr);
	void Ret();

	void Add16(Register16& reg, uint16 val);
	void Add16Signed(Register16& reg, int8 val);
	void Inc16(Register16& reg);
	void Dec16(Register16& reg);

	void SetFlag(uint8 index, bool val) {
		if (val == true) {
			A.Set(BIT_SET(F.Get(), index));
		}
		else {
			A.Set(BIT_UNSET(F.Get(), index));
		}
	}

	void SetZ(bool val) { SetFlag(7, val); }
	void SetN(bool val) { SetFlag(6, val); }
	void SetH(bool val) { SetFlag(5, val); }
	void SetC(bool val) { SetFlag(4, val); }

	bool GetZ() { return BIT_IS_SET(AF.Get(), 7); }
	bool GetN() { return BIT_IS_SET(AF.Get(), 6); }
	bool GetH() { return BIT_IS_SET(AF.Get(), 5); }
	bool GetC() { return BIT_IS_SET(AF.Get(), 4); }

	void Inst0x00();
	void Inst0x01();
	void Inst0x02();
	void Inst0x03();
	void Inst0x04();
	void Inst0x05();
	void Inst0x06();
	void Inst0x07();
	void Inst0x08();
	void Inst0x09();
	void Inst0x0a();
	void Inst0x0b();
	void Inst0x0c();
	void Inst0x0d();
	void Inst0x0e();
	void Inst0x0f();
	void Inst0x10();
	void Inst0x11();
	void Inst0x12();
	void Inst0x13();
	void Inst0x14();
	void Inst0x15();
	void Inst0x16();
	void Inst0x17();
	void Inst0x18();
	void Inst0x19();
	void Inst0x1a();
	void Inst0x1b();
	void Inst0x1c();
	void Inst0x1d();
	void Inst0x1e();
	void Inst0x1f();
	void Inst0x20();
	void Inst0x21();
	void Inst0x22();
	void Inst0x23();
	void Inst0x24();
	void Inst0x25();
	void Inst0x26();
	void Inst0x27();
	void Inst0x28();
	void Inst0x29();
	void Inst0x2a();
	void Inst0x2b();
	void Inst0x2c();
	void Inst0x2d();
	void Inst0x2e();
	void Inst0x2f();
	void Inst0x30();
	void Inst0x31();
	void Inst0x32();
	void Inst0x33();
	void Inst0x34();
	void Inst0x35();
	void Inst0x36();
	void Inst0x37();
	void Inst0x38();
	void Inst0x39();
	void Inst0x3a();
	void Inst0x3b();
	void Inst0x3c();
	void Inst0x3d();
	void Inst0x3e();
	void Inst0x3f();
	void Inst0x40();
	void Inst0x41();
	void Inst0x42();
	void Inst0x43();
	void Inst0x44();
	void Inst0x45();
	void Inst0x46();
	void Inst0x47();
	void Inst0x48();
	void Inst0x49();
	void Inst0x4a();
	void Inst0x4b();
	void Inst0x4c();
	void Inst0x4d();
	void Inst0x4e();
	void Inst0x4f();
	void Inst0x50();
	void Inst0x51();
	void Inst0x52();
	void Inst0x53();
	void Inst0x54();
	void Inst0x55();
	void Inst0x56();
	void Inst0x57();
	void Inst0x58();
	void Inst0x59();
	void Inst0x5a();
	void Inst0x5b();
	void Inst0x5c();
	void Inst0x5d();
	void Inst0x5e();
	void Inst0x5f();
	void Inst0x60();
	void Inst0x61();
	void Inst0x62();
	void Inst0x63();
	void Inst0x64();
	void Inst0x65();
	void Inst0x66();
	void Inst0x67();
	void Inst0x68();
	void Inst0x69();
	void Inst0x6a();
	void Inst0x6b();
	void Inst0x6c();
	void Inst0x6d();
	void Inst0x6e();
	void Inst0x6f();
	void Inst0x70();
	void Inst0x71();
	void Inst0x72();
	void Inst0x73();
	void Inst0x74();
	void Inst0x75();
	void Inst0x76();
	void Inst0x77();
	void Inst0x78();
	void Inst0x79();
	void Inst0x7a();
	void Inst0x7b();
	void Inst0x7c();
	void Inst0x7d();
	void Inst0x7e();
	void Inst0x7f();
	void Inst0x80();
	void Inst0x81();
	void Inst0x82();
	void Inst0x83();
	void Inst0x84();
	void Inst0x85();
	void Inst0x86();
	void Inst0x87();
	void Inst0x88();
	void Inst0x89();
	void Inst0x8a();
	void Inst0x8b();
	void Inst0x8c();
	void Inst0x8d();
	void Inst0x8e();
	void Inst0x8f();
	void Inst0x90();
	void Inst0x91();
	void Inst0x92();
	void Inst0x93();
	void Inst0x94();
	void Inst0x95();
	void Inst0x96();
	void Inst0x97();
	void Inst0x98();
	void Inst0x99();
	void Inst0x9a();
	void Inst0x9b();
	void Inst0x9c();
	void Inst0x9d();
	void Inst0x9e();
	void Inst0x9f();
	void Inst0xa0();
	void Inst0xa1();
	void Inst0xa2();
	void Inst0xa3();
	void Inst0xa4();
	void Inst0xa5();
	void Inst0xa6();
	void Inst0xa7();
	void Inst0xa8();
	void Inst0xa9();
	void Inst0xaa();
	void Inst0xab();
	void Inst0xac();
	void Inst0xad();
	void Inst0xae();
	void Inst0xaf();
	void Inst0xb0();
	void Inst0xb1();
	void Inst0xb2();
	void Inst0xb3();
	void Inst0xb4();
	void Inst0xb5();
	void Inst0xb6();
	void Inst0xb7();
	void Inst0xb8();
	void Inst0xb9();
	void Inst0xba();
	void Inst0xbb();
	void Inst0xbc();
	void Inst0xbd();
	void Inst0xbe();
	void Inst0xbf();
	void Inst0xc0();
	void Inst0xc1();
	void Inst0xc2();
	void Inst0xc3();
	void Inst0xc4();
	void Inst0xc5();
	void Inst0xc6();
	void Inst0xc7();
	void Inst0xc8();
	void Inst0xc9();
	void Inst0xca();
	void Inst0xcb();
	void Inst0xcc();
	void Inst0xcd();
	void Inst0xce();
	void Inst0xcf();
	void Inst0xd0();
	void Inst0xd1();
	void Inst0xd2();
	void Inst0xd3();
	void Inst0xd4();
	void Inst0xd5();
	void Inst0xd6();
	void Inst0xd7();
	void Inst0xd8();
	void Inst0xd9();
	void Inst0xda();
	void Inst0xdb();
	void Inst0xdc();
	void Inst0xdd();
	void Inst0xde();
	void Inst0xdf();
	void Inst0xe0();
	void Inst0xe1();
	void Inst0xe2();
	void Inst0xe3();
	void Inst0xe4();
	void Inst0xe5();
	void Inst0xe6();
	void Inst0xe7();
	void Inst0xe8();
	void Inst0xe9();
	void Inst0xea();
	void Inst0xeb();
	void Inst0xec();
	void Inst0xed();
	void Inst0xee();
	void Inst0xef();
	void Inst0xf0();
	void Inst0xf1();
	void Inst0xf2();
	void Inst0xf3();
	void Inst0xf4();
	void Inst0xf5();
	void Inst0xf6();
	void Inst0xf7();
	void Inst0xf8();
	void Inst0xf9();
	void Inst0xfa();
	void Inst0xfb();
	void Inst0xfc();
	void Inst0xfd();
	void Inst0xfe();
	void Inst0xff();
};
