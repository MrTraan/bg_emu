#include <stdio.h>
#include "gb_emu.h"
#include "cpu.h"

int main()
{
	Register16 r;
	r.Set(0xd00e);
	r.low.mask = 0xF0;

    printf("%x\n", r.Get());
	r.high.Set(0x1d);
    printf("%x\n", r.Get());
	r.low.Set(0xac);
    printf("%x\n", r.Get());
	r.high.Set(0x0d);
    printf("%x\n", r.Get());
	r.high.Set(0x00);
    printf("%x\n", r.Get());
	r.high.Set(0x10);
    printf("%x\n", r.Get());
	r.high.Set(0x20);
    printf("%x\n", r.Get());
	r.low.Set(0x20);
    printf("%x\n", r.Get());
	r.low.Set(0x00);
    printf("%x\n", r.Get());
	r.low.Set(0x0a);
    printf("%x\n", r.Get());
	r.low.Set(0xd1);
    printf("%x\n", r.Get());
    return 0;
}
