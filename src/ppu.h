#pragma once

#include "gb_emu.h"
#include "simple_texture.h"
#include "gui/textured_rectangle.h"

struct Memory;
struct Cpu;
struct Window;

struct Ppu {
	Memory * mem = nullptr;
	Cpu * cpu = nullptr;
	TexturedRectangle frontBuffer;
	TexturedRectangle backBuffer;
	TexturedRectangle * drawingBuffer = nullptr;
	TexturedRectangle * workBuffer = nullptr;

	static bool debugDrawTiles;
	static bool debugDrawSprites;

	int scanlineCounter = 456;
	int lastDrawnScanLine = 0;

	int selectedPalette = 0;
	byte tileScanLine[GB_SCREEN_WIDTH];

	void AllocateBuffers(const Window & window);
	
	void DestroyBuffers() {
		frontBuffer.Destroy();
		backBuffer.Destroy();
		backgroundTexture.Destroy();
		tilesetTexture.Destroy();
	}

	void Reset() {
		frontBuffer.texture.Clear();
		backBuffer.texture.Clear();
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
