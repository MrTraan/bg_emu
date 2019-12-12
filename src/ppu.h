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
		tilesetTexture.Allocate(16 * 8, 24 * 8);
		Reset();
	}
	
	void DestroyBuffers() {
		delete frontBuffer;
		delete backBuffer;
		backgroundTexture.Destroy();
		tilesetTexture.Destroy();
	}

	void Reset() {
		if (frontBuffer)
			frontBuffer->Clear();
		if (backBuffer)
			backBuffer->Clear();
		backgroundTexture.Clear();
		tilesetTexture.Clear();
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
	void DrawFullBackgroundToTexture(SimpleTexture & texture, int width, int height);
	void DrawTilesetToTexture(SimpleTexture & texture);
	SimpleTexture backgroundTexture;
	SimpleTexture tilesetTexture;

	bool drawBackgroundTexture = false;
};
