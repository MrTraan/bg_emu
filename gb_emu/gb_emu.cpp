#include <stdio.h>
#include "gb_emu.h"
#include "cpu.h"
#include "memory.h"

int main()
{
	Memory mem;
	Cpu cpu(&mem);

	while (true) {
		cpu.ExecuteNextOPCode();
	}

	return 0;
}
