#pragma once

#include "gb_emu.h"
#include "gui/screen_buffer.h"

struct Memory;
struct Cpu;

struct Ppu {
	Memory * mem;
	Cpu * cpu;
	ScreenBuffer * frontBuffer;
	ScreenBuffer * backBuffer;

	static bool debugDrawTiles;
	static bool debugDrawSprites;

	int scanlineCounter = 456;
	int lastDrawnScanLine = 0;

	Ppu(Memory * _mem, Cpu * _cpu) : mem(_mem), cpu(_cpu) {
		frontBuffer = new ScreenBuffer;
		backBuffer = new ScreenBuffer;
	}

	~Ppu() {
		delete frontBuffer;
		delete backBuffer;
	}

	void Reset() {
		frontBuffer->Clear();
		backBuffer->Clear();
		scanlineCounter = 456;
		lastDrawnScanLine = 0;
	}

	void SwapBuffers();
	void Update(int cycles);
	bool IsLcdOn();

	void DrawScanLine(int line);
	void DrawTiles(int line, byte scanline);
	void DrawSprites(int line, byte scanline);

	void PutPixel(byte x, byte y, byte tileAttr, byte colorIndex, byte palette);
};
