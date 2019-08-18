#include <string.h>
#include "ppu.h"
#include "cpu.h"
#include "memory.h"

constexpr int lcdMode1Bounds = 144;
constexpr int lcdMode2Bounds = 376;
constexpr int lcdMode3Bounds = 204;

static Pixel paletteData[4] = {
	// Greenishy pallete, like the original shitty gameboy screen
	{0x9B, 0xBC, 0x0F},
	{0x8B, 0xAC, 0x0F},
	{0x30, 0x62, 0x30},
	{0x0F, 0x38, 0x0F},
	/*
	// Grey scale palette
	{0xFF, 0xFF, 0xFF},
	{0xCC, 0xCC, 0xCC},
	{0x77, 0x77, 0x77},
	{0x00, 0x00, 0x00},
	*/
};

bool Ppu::IsLcdOn() {
	return BIT_IS_SET(mem->Read(0xff40), 7);
}

void Ppu::Update(int cycles) {
	byte status = mem->Read(0xff41);

	if (!IsLcdOn()) {
		backBuffer->Clear();
		SwapBuffers();

		scanlineCounter = 456;
		status &= 0xfc;
		status = BIT_UNSET(status, 0);
		status = BIT_UNSET(status, 1);
		mem->Write(0xff41, status);
		return;
	}
	byte currentLine = mem->Read(0xff44);
	byte currentMode = status & 0x3;
	byte nextMode = 0;
	bool requestInterupt = false;

	if (currentLine >= 144) {
		nextMode = 1;
		status = BIT_SET(status, 0);
		status = BIT_UNSET(status, 1);
		requestInterupt = BIT_IS_SET(status, 4);
	} else {
		if (scanlineCounter >= lcdMode2Bounds) {
			if (currentLine != lastDrawnScanLine) {
				DrawScanLine(lastDrawnScanLine);
				lastDrawnScanLine = currentLine;
			}
			nextMode = 2;
			status = BIT_UNSET(status, 0);
			status = BIT_SET(status, 1);
			requestInterupt = BIT_IS_SET(status, 5);
		}
		else if (scanlineCounter >= lcdMode3Bounds) {
			nextMode = 3;
			status = BIT_SET(status, 0);
			status = BIT_SET(status, 1);
		}
		else {
			nextMode = 0;
			status = BIT_UNSET(status, 0);
			status = BIT_UNSET(status, 1);
			requestInterupt = BIT_IS_SET(status, 3);
			if (nextMode != currentMode) {
				mem->HDMATransfer();
			}
		}
	}

	if (requestInterupt && nextMode != currentMode) {
		cpu->RaiseInterupt(1);
	}

	if (currentLine == mem->Read(0xff45)) {
		status = BIT_SET(status, 2);
		if (BIT_IS_SET(status, 6)) {
			cpu->RaiseInterupt(1);
		}
	}
	else {
		status = BIT_UNSET(status, 2);
	}

	mem->Write(0xff41, status);

	////////////////

	scanlineCounter -= cycles;

	if (scanlineCounter <= 0) {
		mem->Write(0xff44, mem->Read(0xff44) + 1);
		byte currentLine = mem->Read(0xff44);
		if (currentLine > 153) {
			SwapBuffers();
			mem->Write(0xff44, 0);
		}

		scanlineCounter += 456 * cpu->speed;
		if (currentLine == GB_SCREEN_HEIGHT) {
			cpu->RaiseInterupt(0);
		}
	}
}

void Ppu::DrawScanLine(int scanline) {
	byte control = mem->Read(0xff40);

	if (cpu->IsCGB() || BIT_IS_SET(control, 0)) {
		DrawTiles(scanline, control);
	}

	if (BIT_IS_SET(control, 1)) {
		DrawSprites(scanline, control);
	}
}

void Ppu::PutPixel(byte x, byte y, byte tileAttr, byte colorIndex, byte palette) {
	if (cpu->IsCGB()) {
		DEBUG_BREAK;
	}
	else {
		byte highBit = colorIndex << 1 | 1;
		byte lowBit = colorIndex << 1;
		byte column = (BIT_VALUE(palette, highBit) << 1) | BIT_VALUE(palette, lowBit);
		Pixel & pixel = paletteData[column];
		backBuffer->SetPixel(x, y, pixel);
	}
}


void Ppu::SwapBuffers() {
	ScreenBuffer* tmp = frontBuffer;
	frontBuffer = backBuffer;
	backBuffer = tmp;
	backBuffer->Clear();
}

void Ppu::DrawTiles(int scanline, byte control) {
	// Draw tiles
	byte scrollY = mem->Read(0xff42);
	byte scrollX = mem->Read(0xff43);
	byte windowY = mem->Read(0xff4a);
	byte windowX = mem->Read(0xff4b) - 7;

	uint16 tileData = 0x8800;
	bool usingUnsigned = false;
	bool usingWindow = false;
	if (BIT_IS_SET(control, 4)) {
		tileData = 0x8000;
		usingUnsigned = true;
	}
	if (BIT_IS_SET(control, 5)) {
		if (mem->Read(0xff44) > windowY) {
			usingWindow = true; // Is current scanline inside the window?
		}
	}
	uint16 backgroundMemory = 0x9800;
	if (BIT_IS_SET(control, (usingWindow ? 6 : 3))) {
		backgroundMemory = 0x9c00; // switching to window memory
	}

	byte yPos = usingWindow ? scanline - windowY : scanline + scrollY;
	byte tileRow = yPos / 8 * 32;
	byte palette = mem->Read(0xff47);

	byte tileScanLine[GB_SCREEN_WIDTH];
	memset(tileScanLine, 0, GB_SCREEN_WIDTH);
	// Draw one horizontal line
	for (byte x = 0; x < GB_SCREEN_WIDTH; x++) {
		byte xPos = (usingWindow && x >= windowX) ? x - windowX : x + scrollX;
		uint16 tileColumn = xPos / 8;
		uint16 tileAddr = backgroundMemory + tileRow + tileColumn;

		uint16 tileLocation;
		if (usingUnsigned) {
			uint16 tileIndex = (uint16)(mem->VRAM[tileAddr - 0x8000]); // @HARDCODED this should use mem->Read()
			tileLocation = tileData + (tileIndex * 16);
		}
		else {
			int16 tileIndex = (int8)(mem->VRAM[tileAddr - 0x8000]); // @HARDCODED this should use mem->Read()
			tileLocation = (uint16)((int)tileData + (int)((tileIndex + 128) * 16));
		}

		// Attributes used in CGB mode
		//
		//    Bit 0-2  Background Palette number  (BGP0-7)
		//    Bit 3    Tile VRAM Bank number      (0=Bank 0, 1=Bank 1)
		//    Bit 5    Horizontal Flip            (0=Normal, 1=Mirror horizontally)
		//    Bit 6    Vertical Flip              (0=Normal, 1=Mirror vertically)
		//    Bit 7    BG-to-OAM Priority         (0=Use OAM priority bit, 1=BG Priority)

		byte tileAttr = mem->VRAM[tileAddr - 0x6000];
		bool useBank1 = BIT_IS_SET(tileAttr, 3);
		bool hflip = BIT_IS_SET(tileAttr, 5);
		bool vflip = BIT_IS_SET(tileAttr, 6);
		bool priority = BIT_IS_SET(tileAttr, 7);

		uint16 bankOffset = cpu->IsCGB() && useBank1 ? 0x6000 : 0x8000;
		byte line = cpu->IsCGB() && vflip ? ((7 - yPos) % 8) * 2 : (yPos % 8) * 2;

		byte tileData1 = mem->VRAM[tileLocation + line - bankOffset];
		byte tileData2 = mem->VRAM[tileLocation + line - bankOffset + 1];

		if (cpu->IsCGB() && hflip) {
			xPos = 7 - xPos;
		}
		byte colorBit = (byte)((int8)((xPos % 8) - 7) * -1);
		byte colorIndex = (BIT_VALUE(tileData2, colorBit) << 1) | BIT_VALUE(tileData1, colorBit);
		// Draw if sprite has priority of if no pixel has been drawn there
		if (priority || tileScanLine[x] == 0) {
			PutPixel(x, scanline, tileAttr, colorIndex, palette);
		}
	}
}

void Ppu::DrawSprites(int scanline, byte control) {
	int ySize = BIT_IS_SET(control, 2) ? 16 : 8;
	byte palette1 = mem->Read(0xFF48);
	byte palette2 = mem->Read(0xFF49);

	int minX[GB_SCREEN_WIDTH];
	memset(minX, 0, sizeof(minX));
	int lineSprites = 0;
	for (uint16 sprite = 0; sprite < 40; sprite++) {
		uint16 index = sprite * 4;

		int yPos = mem->Read(0xfe00 + index) - 16;
		if (scanline < yPos || scanline >= (yPos + ySize)) {
			continue;
		}

		if (lineSprites >= 10) {
			break; // Game boy can't display more than 10 sprites per line
		}
		lineSprites++;

		int xPos = mem->Read(0xfe00 + index + 1) - 8;
		int tileLocation = mem->Read(0xfe00 + index + 2);
		int spriteAttr = mem->Read(0xfe00 + index + 3);

		bool useBank1 = BIT_IS_SET(spriteAttr, 3);
		bool hflip = BIT_IS_SET(spriteAttr, 5);
		bool vflip = BIT_IS_SET(spriteAttr, 6);
		bool priority = BIT_IS_SET(spriteAttr, 7);

		uint16 bankOffset = cpu->IsCGB() && useBank1 ? 0x2000 : 0x0;

		int line = scanline - yPos;
		if (vflip) {
			line = ySize - line - 1;
		}

		uint16 dataAddr = ((uint16)tileLocation * 16) + line * 2 + bankOffset;
		byte spriteData1 = mem->VRAM[dataAddr];
		byte spriteData2 = mem->VRAM[dataAddr + 1];

		// Draw the sprite line
		for (byte tilePixel = 0; tilePixel < 8; tilePixel++) {
			int16 pixel = xPos + int16(7 - tilePixel);
			if (pixel < 0 || pixel >= GB_SCREEN_WIDTH) {
				continue;
			}
			// Check if something was already drawn here
			if (minX[pixel] != 0 && (cpu->IsCGB() || minX[pixel] <= xPos + 100)) {
				continue;
			}

			byte colorBit = hflip ? (byte)((int8)(colorBit - 7) * -1) : tilePixel;
			byte colorIndex = (BIT_VALUE(spriteData2, colorBit) << 1) | BIT_VALUE(spriteData1, colorBit);

			if (colorIndex == 0) {
				// transparent pixel
				continue;
			}

			if (cpu->IsCGB()) {
				DEBUG_BREAK;
			} else {
				PutPixel(pixel, (byte)scanline, spriteAttr, colorIndex, BIT_IS_SET(spriteAttr, 4) ? palette2 : palette1);
			}
		}
	}
}