#pragma once

#include "gb_emu.h"
#include "gui/screen_buffer.h"

struct Memory;
struct Cpu;

struct Ppu {
	Memory * mem = nullptr;
	Cpu * cpu = nullptr;
	ScreenBuffer * frontBuffer = nullptr;
	ScreenBuffer * backBuffer = nullptr;

	static bool debugDrawTiles;
	static bool debugDrawSprites;

	int scanlineCounter = 456;
	int lastDrawnScanLine = 0;

	int selectedPalette = 0;
	byte tileScanLine[GB_SCREEN_WIDTH];

	void AllocateBuffers() {
		frontBuffer = new ScreenBuffer;
		backBuffer = new ScreenBuffer;
		Reset();
	}
	
	void DestroyBuffers() {
		delete frontBuffer;
		delete backBuffer;
	}

	void Reset() {
		if (frontBuffer)
			frontBuffer->Clear();
		if (backBuffer)
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

	void DebugDraw();
	void DrawTilesetToTexture(Pixel * texture, int width, int height);
	Pixel * backgroundTexture = nullptr;
	bool drawBackgroundTexture = true;
	unsigned int backgroundTextureHandler = 0;
};
