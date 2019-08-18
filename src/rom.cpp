#include <stdio.h>
#include "rom.h"

Cartridge * Cartridge::LoadFromFile(const char * path) {
	FILE * fh = fopen(path, "r");
	if (fh == nullptr) {
		printf("Could not find ROM file %s\n", path);
		DEBUG_BREAK;
		return nullptr;
	}
	fseek(fh, 0L, SEEK_END);
	long size = ftell(fh);
	rewind(fh);

	if (size > 0x8000) {
		DEBUG_BREAK;
		printf("Unhandled ROM file, the file is too large\n");
		fclose(fh);
		return nullptr;
	}

	ROM * rom = new ROM();

	fread(rom->data, size, 1, fh);
	fclose(fh);

	return rom;
}