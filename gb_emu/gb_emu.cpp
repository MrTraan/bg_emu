#include <stdio.h>
#include "gb_emu.h"
#include "cpu.h"

int main()
{
	Register r;
	r.Set(0);
	r.mask = 0xFFF0;

	r.SetHigh(0x1d);
    printf("%x\n", r.Get());
	r.SetLow(0xac);
    printf("%x\n", r.Get());
	r.SetHigh(0x0d);
    printf("%x\n", r.Get());
	r.SetHigh(0x00);
    printf("%x\n", r.Get());
	r.SetHigh(0x10);
    printf("%x\n", r.Get());
	r.SetHigh(0x20);
    printf("%x\n", r.Get());
	r.SetLow(0x20);
    printf("%x\n", r.Get());
	r.SetLow(0x00);
    printf("%x\n", r.Get());
	r.SetLow(0x0a);
    printf("%x\n", r.Get());
	r.SetLow(0xd1);
    printf("%x\n", r.Get());
    return 0;
}
