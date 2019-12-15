#pragma once

#include "gb_emu.h"
#include "simple_texture.h"
#include "gui/textured_rectangle.h"

struct Gameboy;
struct Window;

struct Ppu {
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
	void Update(int cycles, Gameboy * gb);
	bool IsLcdOn(Gameboy * gb);

	void DrawScanLine(int line, Gameboy * gb);
	void DrawTiles(int line, byte scanline, Gameboy * gb);
	void DrawSprites(int line, byte scanline, Gameboy * gb);

	void PutPixel(byte x, byte y, byte tileAttr, byte colorIndex, byte palette, bool priority, Gameboy * gb);

	void DebugDraw(Gameboy * gb);
	void DrawFullBackgroundToTexture(SimpleTexture & texture, int width, int height, Gameboy * gb);
	void DrawTilesetToTexture(SimpleTexture & texture, Gameboy * gb);
	SimpleTexture backgroundTexture;
	SimpleTexture tilesetTexture;

	bool drawBackgroundTexture = false;
};
