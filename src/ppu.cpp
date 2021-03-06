#include <string.h>
#include "ppu.h"
#include "cpu.h"
#include "gameboy.h"
#include <imgui/imgui.h>
#include "gui/window.h"

constexpr int lcdMode1Bounds = 144;
constexpr int lcdMode2Bounds = 376;
constexpr int lcdMode3Bounds = 204;

static Pixel dmgPaletteColors[3][4] = {
	{
		// Greenishy pallete, like the original shitty gameboy screen
		{0x9B, 0xBC, 0x0F},
		{0x8B, 0xAC, 0x0F},
		{0x30, 0x62, 0x30},
		{0x0F, 0x38, 0x0F},
	},
	{
		// Grey scale palette
		{0xFF, 0xFF, 0xFF},
		{0xCC, 0xCC, 0xCC},
		{0x77, 0x77, 0x77},
		{0x00, 0x00, 0x00},
	},
	{
		// Blueish palette
		{0xE0, 0xF8, 0xD0},
		{0x88, 0xC0, 0x70},
		{0x34, 0x68, 0x56},
		{0x08, 0x18, 0x20},
	},
};

uint8 cgbColorsValue[] = {
	0x0,  0x8,	0x10, 0x18, 0x20, 0x29, 0x31, 0x39, 0x41, 0x4a, 0x52, 0x5a, 0x62, 0x6a, 0x73, 0x7b,
	0x83, 0x8b, 0x94, 0x9c, 0xa4, 0xac, 0xb4, 0xbd, 0xc5, 0xcd, 0xd5, 0xde, 0xe6, 0xee, 0xf6, 0xff,
};

void Ppu::AllocateBuffers( const Window & window ) {
	frontBuffer.Allocate(0, 20, GB_SCREEN_WIDTH * 4, GB_SCREEN_HEIGHT * 4, window);
	backBuffer.Allocate(0, 20, GB_SCREEN_WIDTH * 4, GB_SCREEN_HEIGHT * 4, window);
	frontBuffer.texture.Allocate(GB_SCREEN_WIDTH, GB_SCREEN_HEIGHT);
	backBuffer.texture.Allocate(GB_SCREEN_WIDTH, GB_SCREEN_HEIGHT);
	workBuffer = &frontBuffer;
	drawingBuffer = &backBuffer;
	backgroundTexture.Allocate(256, 256);
	tilesetTexture.Allocate(16 * 8, 24 * 8);
	Reset();
}

bool Ppu::IsLcdOn(Gameboy * gb) {
	return BIT_IS_SET(gb->Read(0xff40), 7);
}

void Ppu::Update(int cycles, Gameboy * gb) {
	byte status = gb->Read(0xff41);

	if (!IsLcdOn(gb)) {
		scanlineCounter = 456;
		gb->mem.highRAM[0x44] = 0;
		status &= 0xfc;
		status = BIT_UNSET(status, 0);
		status = BIT_UNSET(status, 1);
		gb->Write(0xff41, status);
		return;
	}
	byte currentLine = gb->Read(0xff44);
	byte currentMode = status & 0x3;
	byte nextMode = 0;
	bool requestInterupt = false;

	if (currentLine >= 144) {
		nextMode = 1;
		status = BIT_SET(status, 0);
		status = BIT_UNSET(status, 1);
		requestInterupt = BIT_IS_SET(status, 4);
	} else if (scanlineCounter >= lcdMode2Bounds) {
		nextMode = 2;
		status = BIT_UNSET(status, 0);
		status = BIT_SET(status, 1);
		requestInterupt = BIT_IS_SET(status, 5);
	}
	else if (scanlineCounter >= lcdMode3Bounds) {
		nextMode = 3;
		status = BIT_SET(status, 0);
		status = BIT_SET(status, 1);
		if (nextMode != currentMode) {
			DrawScanLine(currentLine, gb);
		}
	}
	else {
		nextMode = 0;
		status = BIT_UNSET(status, 0);
		status = BIT_UNSET(status, 1);
		requestInterupt = BIT_IS_SET(status, 3);
		if (nextMode != currentMode) {
			gb->HDMATransfer();
		}
	}

	if (requestInterupt && nextMode != currentMode) {
		gb->RaiseInterupt(1);
	}

	if (currentLine == gb->Read(0xff45)) {
		status = BIT_SET(status, 2);
		if (BIT_IS_SET(status, 6)) {
			gb->RaiseInterupt(1);
		}
	}
	else {
		status = BIT_UNSET(status, 2);
	}

	gb->Write(0xff41, status);

	////////////////

	scanlineCounter -= cycles;

	if (scanlineCounter <= 0) {
		byte currentLine = gb->Read(0xff44) + 1;
		gb->Write(0xff44, currentLine);
		if (currentLine > 153) {
			SwapBuffers();
			memset(bgPriority, 0, sizeof(bgPriority));
			gb->Write(0xff44, 0);
			currentLine = 0;
		}

		scanlineCounter += 456 * gb->cpu.speed;
		if (currentLine == GB_SCREEN_HEIGHT) {
			gb->RaiseInterupt(0);
		}
	}
}

void Ppu::DrawScanLine(int scanline, Gameboy * gb) {
	byte control = gb->Read(0xff40);

	if ((gb->cpu.IsCGB || BIT_IS_SET(control, 0)) && debugDrawTiles) {
		DrawTiles(scanline, control, gb);
	}

	if (BIT_IS_SET(control, 1) && debugDrawSprites) {
		DrawSprites(scanline, control, gb);
	}
}

void Ppu::PutPixel(byte x, byte y, byte tileAttr, byte colorIndex, byte palette, bool priority, Gameboy * gb, const CGBPalette & CGBpalette) {
	Pixel pixel;
	if (gb->cpu.IsCGB) {
		byte cgbPalette = tileAttr & 0x7;
		byte index = cgbPalette * 8 + colorIndex * 2;
		uint16 color = CGBpalette.palette[index] | (CGBpalette.palette[index + 1] << 8);
		pixel.R = cgbColorsValue[ color & 0x1f ];
		pixel.G = cgbColorsValue[ ( color >> 5 ) & 0x1f ];
		pixel.B = cgbColorsValue[ ( color >> 10 ) & 0x1f ];
	}
	else {
		byte highBit = colorIndex << 1 | 1;
		byte lowBit = colorIndex << 1;
		byte column = (BIT_VALUE(palette, highBit) << 1) | BIT_VALUE(palette, lowBit);
		pixel = dmgPaletteColors[selectedPalette][column];
	}
	if ( (priority && bgPriority[x + y * GB_SCREEN_WIDTH] == 0 ) || tileScanLine[x] == 0 ) {
		workBuffer->texture.SetPixel(pixel, x, y);
	}
}

void Ppu::SwapBuffers() {
	if ( workBuffer == &frontBuffer) {
		workBuffer = &backBuffer;
		drawingBuffer = &frontBuffer;
	} else {
		workBuffer = &frontBuffer;
		drawingBuffer = &backBuffer;
	}
	workBuffer->texture.Clear();
}

void Ppu::DrawTiles(int scanline, byte control, Gameboy * gb) {
	// Draw tiles
	byte scrollY = gb->Read(0xff42);
	byte scrollX = gb->Read(0xff43);
	byte windowY = gb->Read(0xff4a);
	byte windowX = gb->Read(0xff4b) - 7;

	uint16 tileData = 0x8800;
	bool usingUnsigned = false;
	bool usingWindow = false;
	if (BIT_IS_SET(control, 4)) {
		tileData = 0x8000;
		usingUnsigned = true;
	}
	if (BIT_IS_SET(control, 5)) {
		if (gb->Read(0xff44) >= windowY) {
			usingWindow = true; // Is current scanline inside the window?
		}
	}
	uint16 backgroundMemory = 0x9800;
	if (BIT_IS_SET(control, (usingWindow ? 6 : 3))) {
		backgroundMemory = 0x9c00; // switching to window memory
	}

	byte yPos = usingWindow ? scanline - windowY : scanline + scrollY;
	uint16 tileRow = (uint16)(yPos / 8) * 32;
	byte palette = gb->Read(0xff47);

	memset(tileScanLine, 0, sizeof(tileScanLine));
	// Draw one horizontal line
	for (byte x = 0; x < GB_SCREEN_WIDTH; x++) {
		byte xPos = (usingWindow && x >= windowX) ? x - windowX : x + scrollX;
		uint16 tileColumn = xPos / 8;
		uint16 tileAddr = backgroundMemory + tileRow + tileColumn;

		uint16 tileLocation;
		if (usingUnsigned) {
			int16 tileIndex = (int16)(gb->Read(tileAddr)); 
			tileLocation = tileData + (uint16)(tileIndex * 16) + 0x0000;
		}
		else {
			int16 tileIndex = (int8)(gb->Read(tileAddr));
			tileLocation = (uint16)((int)tileData + (int)((tileIndex + 128) * 16)) + 0x0000;
		}

		// Attributes used in CGB mode
		//
		//    Bit 0-2  Background Palette number  (BGP0-7)
		//    Bit 3    Tile VRAM Bank number      (0=Bank 0, 1=Bank 1)
		//    Bit 5    Horizontal Flip            (0=Normal, 1=Mirror horizontally)
		//    Bit 6    Vertical Flip              (0=Normal, 1=Mirror vertically)
		//    Bit 7    BG-to-OAM Priority         (0=Use OAM priority bit, 1=BG Priority)

		byte tileAttr = gb->Read(tileAddr + 0x2000);
		bool useBank1 = BIT_IS_SET(tileAttr, 3);
		bool hflip = BIT_IS_SET(tileAttr, 5);
		bool vflip = BIT_IS_SET(tileAttr, 6);
		bool priority = BIT_IS_SET(tileAttr, 7);

		uint16 bankOffset = gb->cpu.IsCGB && useBank1 ? 0x2000 : 0x0000;
		byte line = gb->cpu.IsCGB && vflip ? ((7 - yPos) % 8) * 2 : (yPos % 8) * 2;

		byte tileData1 = gb->Read(tileLocation + line + bankOffset);
		byte tileData2 = gb->Read(tileLocation + line + bankOffset + 1);

		if (gb->cpu.IsCGB && hflip) {
			xPos = 7 - xPos;
		}
		byte colorBit = (int8)((xPos % 8) - 7) * -1;
		byte colorIndex = (BIT_VALUE(tileData2, colorBit) << 1) | BIT_VALUE(tileData1, colorBit);
		// Draw if sprite has priority of if no pixel has been drawn there
		PutPixel(x, scanline, tileAttr, colorIndex, palette, true, gb, gb->mem.bgPalette);
		tileScanLine[x] = colorIndex;
		if (gb->cpu.IsCGB) {
			bgPriority[x + scanline * GB_SCREEN_WIDTH ] = priority ? 1 : 0;
		}
	}
}

void Ppu::DrawSprites(int scanline, byte control, Gameboy * gb) {
	int ySize = BIT_IS_SET(control, 2) ? 16 : 8;
	byte palette1 = gb->Read(0xFF48);
	byte palette2 = gb->Read(0xFF49);

	int minX[GB_SCREEN_WIDTH];
	memset(minX, 0, sizeof(minX));
	int lineSprites = 0;
	for (uint16 sprite = 0; sprite < 40; sprite++) {
		uint16 index = sprite * 4;

		int yPos = (int)(gb->Read(0xfe00 + index)) - 16;
		if (scanline < yPos || scanline >= (yPos + ySize)) {
			continue;
		}

		if (lineSprites >= 10) {
			break; // Game boy can't display more than 10 sprites per line
		}
		lineSprites++;

		int xPos = (int)(gb->Read(0xfe00 + index + 1)) - 8;
		int tileLocation = gb->Read(0xfe00 + index + 2);
		int spriteAttr = gb->Read(0xfe00 + index + 3);

		bool useBank1 = BIT_IS_SET(spriteAttr, 3);
		bool hflip = BIT_IS_SET(spriteAttr, 5);
		bool vflip = BIT_IS_SET(spriteAttr, 6);
		bool priority = !BIT_IS_SET(spriteAttr, 7);

		uint16 bankOffset = gb->cpu.IsCGB && useBank1 ? 0x2000 : 0x0;

		int line = scanline - yPos;
		if (vflip) {
			line = ySize - line - 1;
		}

		uint16 dataAddr = ((uint16)tileLocation * 16) + (line * 2) + bankOffset;
		byte spriteData1 = gb->mem.VRAM[dataAddr];
		byte spriteData2 = gb->mem.VRAM[dataAddr + 1];

		// Draw the sprite line
		for (byte tilePixel = 0; tilePixel < 8; tilePixel++) {
			int16 pixel = xPos + int16(7 - tilePixel);
			if (pixel < 0 || pixel >= GB_SCREEN_WIDTH) {
				continue;
			}
			// Check if something was already drawn here
			if (minX[pixel] != 0 && (gb->cpu.IsCGB || minX[pixel] <= xPos + 100)) {
				continue;
			}

			byte colorBit = hflip ? (byte)((int8)(tilePixel - 7) * -1) : tilePixel;
			byte colorIndex = (BIT_VALUE(spriteData2, colorBit) << 1) | BIT_VALUE(spriteData1, colorBit);

			if (colorIndex == 0) {
				// transparent pixel
				continue;
			}

			PutPixel((byte)pixel, (byte)scanline, spriteAttr, colorIndex, BIT_IS_SET(spriteAttr, 4) ? palette2 : palette1, priority, gb, gb->mem.spritePalette);

			minX[pixel] = xPos + 100;
		}
	}
}

void Ppu::DebugDraw(Gameboy * gb) {
	//ImGui::Image((void*)(ppu->frontBuffer->textureHandler), ImVec2(GB_SCREEN_WIDTH, GB_SCREEN_HEIGHT), ImVec2(0,0), ImVec2(1,1), ImVec4(1.0f,1.0f,1.0f,1.0f), ImVec4(1.0f,1.0f,1.0f,0.5f));
	const char * palettesNames[] = { "Green", "Grey", "Blue" };
	ImGui::Combo("Palette theme", &selectedPalette, palettesNames, 3);
	ImGui::Checkbox( "Draw tiles", &(Ppu::debugDrawTiles) );
	ImGui::SameLine();
	ImGui::Checkbox( "Draw sprites", &(Ppu::debugDrawSprites) );
	byte scrollY = gb->Read(0xff42);
	byte scrollX = gb->Read(0xff43);
	ImGui::Text("Scroll X %d Scroll Y %d", scrollX, scrollY);
	ImGui::Text("Scanline counter: %d", scanlineCounter);
	byte currentLine = gb->Read(0xff44);
	ImGui::Text("Current line: %d", currentLine);

	ImGui::Checkbox( "Draw background texture", &drawBackgroundTexture );
	if (drawBackgroundTexture) {
		DrawFullBackgroundToTexture(backgroundTexture, backgroundTexture.width, backgroundTexture.height, gb);
		backgroundTexture.Update();
		ImGui::Image((void*)(backgroundTexture.textureHandler), ImVec2((float)backgroundTexture.width, (float)backgroundTexture.height), ImVec2(0,0), ImVec2(1,1), ImVec4(1.0f,1.0f,1.0f,1.0f), ImVec4(1.0f,1.0f,1.0f,0.5f));
	}
	static bool drawTileset = false;
	ImGui::Checkbox( "Draw tileset", &drawTileset);
	if (drawTileset) {
		tilesetTexture.Clear();
		DrawTilesetToTexture(tilesetTexture, gb);
		tilesetTexture.Update();
		ImGui::Image((void*)(tilesetTexture.textureHandler), ImVec2((float)tilesetTexture.width, (float)tilesetTexture.height), ImVec2(0,0), ImVec2(1,1), ImVec4(1.0f,1.0f,1.0f,1.0f), ImVec4(1.0f,1.0f,1.0f,0.5f));
	}
	
}

void Ppu::DrawFullBackgroundToTexture(SimpleTexture & texture, int width, int height, Gameboy * gb) {
	byte scrollY = gb->Read(0xff42);
	byte scrollX = gb->Read(0xff43);
	byte control = gb->Read(0xff40);
	uint16 tileData = 0x8800;
	bool usingUnsigned = false;
	if ( BIT_IS_SET( control, 4 ) ) {
		tileData = 0x8000;
		usingUnsigned = true;
	}
	uint16 backgroundMemory = 0x9800;
	if ( BIT_IS_SET( control, 3 ) ) {
		backgroundMemory = 0x9c00; // switching to window memory
	}

	for ( int yPos = 0; yPos < height; yPos++ ) {
		uint16	tileRow = ( uint16 )( yPos / 8 ) * 32;
		byte	palette = gb->Read( 0xff47 );

		// Draw one horizontal line
		for ( int x = 0; x < width; x++ ) {
			byte	xPos = x;
			uint16	tileColumn = xPos / 8;
			uint16	tileAddr = backgroundMemory + tileRow + tileColumn;

			uint16 tileLocation;
			if ( usingUnsigned ) {
				int16 tileIndex = ( int16 )( gb->mem.VRAM[ tileAddr - 0x8000 ] ); // @HARDCODED this should use gb->Read()
				tileLocation = tileData + ( uint16 )( tileIndex * 16 );
			} else {
				int16 tileIndex = ( int8 )( gb->mem.VRAM[ tileAddr - 0x8000 ] ); // @HARDCODED this should use gb->Read()
				tileLocation = ( uint16 )( (int)tileData + (int)( ( tileIndex + 128 ) * 16 ) );
			}

			// Attributes used in CGB mode
			//
			//    Bit 0-2  Background Palette number  (BGP0-7)
			//    Bit 3    Tile VRAM Bank number      (0=Bank 0, 1=Bank 1)
			//    Bit 5    Horizontal Flip            (0=Normal, 1=Mirror horizontally)
			//    Bit 6    Vertical Flip              (0=Normal, 1=Mirror vertically)
			//    Bit 7    BG-to-OAM Priority         (0=Use OAM priority bit, 1=BG Priority)

			byte tileAttr = gb->mem.VRAM[ tileAddr - 0x6000 ];
			bool useBank1 = BIT_IS_SET( tileAttr, 3 );
			bool hflip = BIT_IS_SET( tileAttr, 5 );
			bool vflip = BIT_IS_SET( tileAttr, 6 );
			bool priority = BIT_IS_SET( tileAttr, 7 );

			uint16	bankOffset = gb->cpu.IsCGB && useBank1 ? 0x6000 : 0x8000;
			byte	line = gb->cpu.IsCGB && vflip ? ( ( 7 - yPos ) % 8 ) * 2 : ( yPos % 8 ) * 2;

			byte tileData1 = gb->mem.VRAM[ tileLocation + line - bankOffset ];
			byte tileData2 = gb->mem.VRAM[ tileLocation + line - bankOffset + 1 ];

			if ( gb->cpu.IsCGB && hflip ) {
				xPos = 7 - xPos;
			}
			byte	colorBit = ( int8 )( ( xPos % 8 ) - 7 ) * -1;
			byte	colorIndex = ( BIT_VALUE( tileData2, colorBit ) << 1 ) | BIT_VALUE( tileData1, colorBit );
			byte	highBit = colorIndex << 1 | 1;
			byte	lowBit = colorIndex << 1;
			byte	column = ( BIT_VALUE( palette, highBit ) << 1 ) | BIT_VALUE( palette, lowBit );
			int		boundXMin = MIN( scrollX, ( scrollX + GB_SCREEN_WIDTH ) % width );
			int		boundYMin = MIN( scrollY, ( scrollY + GB_SCREEN_HEIGHT ) % height );
			int		boundXMax = MAX( scrollX, ( scrollX + GB_SCREEN_WIDTH ) % width );
			int		boundYMax = MAX( scrollY, ( scrollY + GB_SCREEN_HEIGHT ) % height );
			if ( ( ( x == boundXMin || x == boundXMax ) && yPos >= boundYMin && yPos <= boundYMax ) ||
				 ( ( yPos == boundYMin || yPos == boundYMax ) && x >= boundXMin && x <= boundXMax ) ) {
				texture.SetPixel( Pixel{ 0, 0, 0 }, xPos, yPos );
			} else {
				texture.SetPixel( dmgPaletteColors[ selectedPalette ][ column ], xPos, yPos );
			}
		}
	}
}

void Ppu::DrawTilesetToTexture(SimpleTexture & texture, Gameboy * gb) {
	int		x = 0;
	int		y = 0;
	byte	palette = gb->Read( 0xff47 );
	int		line = 0;
	for ( int i = 0x8000; i < 0x9800; i += 2 ) {
		byte color1 = gb->Read( i );
		byte color2 = gb->Read( i + 1 );
		for ( int j = 7; j >= 0; j-- ) {
			byte	colorIndex = ( BIT_VALUE( color2, j ) << 1 ) | ( BIT_VALUE( color1, j ) );
			byte	highBit = colorIndex << 1 | 1;
			byte	lowBit = colorIndex << 1;
			byte	column = ( BIT_VALUE( palette, highBit ) << 1 ) | BIT_VALUE( palette, lowBit );
			Pixel & pixel = dmgPaletteColors[ selectedPalette ][ column ];
			texture.SetPixel( pixel, x++, y );
		}
		line++;
		if ( line == 8 ) {
			line = 0;
			if ( x >= 16 * 8 ) {
				x = 0;
				y++;
			} else {
				y = y - ( y % 8 );
			}
		} else {
			x--;
			x = x - ( x % 8 );
			y++;
		}
	}
}

bool Ppu::debugDrawSprites = true;
bool Ppu::debugDrawTiles = true;