#pragma once
#include <string.h>
#include "../gb_emu.h"

struct ScreenBuffer {
	unsigned int VBO;
	unsigned int VAO;
	unsigned int EBO;
	unsigned int textureHandler;

	Pixel textureData[GB_SCREEN_HEIGHT][GB_SCREEN_WIDTH];

	void SetPixel(int x, int y, Pixel & pixel) {
		textureData[y][x] = pixel;
	}
	void Clear() {
		memset(textureData, 0, sizeof(textureData));
	}

	int vertexShader;
	int fragmentShader;
	int shaderProgram;

	ScreenBuffer();
	~ScreenBuffer();

	void Draw();
};