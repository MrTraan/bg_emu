#pragma once

#include "gb_emu.h"
#include "gui/screen_buffer.h"
#include "simple_texture.h"

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
		backgroundTexture.Allocate(256, 256);
		Reset();
	}
	
	void DestroyBuffers() {
		delete frontBuffer;
		delete backBuffer;
		backgroundTexture.Destroy();
	}

	void Reset() {
		if (frontBuffer)
			frontBuffer->Clear();
		if (backBuffer)
			backBuffer->Clear();
		backgroundTexture.Clear();
		scanlineCounter = 456;
		lastDrawnScanLine = 0;
	}

	void SwapBuffers();
	void Update(int cycles);
	bool IsLcdOn();

	void DrawScanLine(int line);
	void DrawTiles(int line, byte scanline);
	void DrawSprites(int line, byte scanline);

	void PutPixel(byte x, byte y, byte tileAttr, byte colorIndex, byte palette, bool priority);

	void DebugDraw();
	void DrawTilesetToTexture(SimpleTexture & texture, int width, int height);
	SimpleTexture backgroundTexture;

	bool drawBackgroundTexture = false;
};
