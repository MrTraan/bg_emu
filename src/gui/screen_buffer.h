#pragma once
#include "../gb_emu.h"

constexpr int SCREEN_BUFFER_HEIGHT = 144;
constexpr int SCREEN_BUFFER_WIDTH = 160;

struct Pixel {
	byte R;
	byte G;
	byte B;
};

struct ScreenBuffer {
	unsigned int VBO;
	unsigned int VAO;
	unsigned int EBO;
	unsigned int textureHandler;

	Pixel textureData[SCREEN_BUFFER_HEIGHT][SCREEN_BUFFER_WIDTH];

	void SetPixel(int x, int y, Pixel & pixel) {
		textureData[y][x] = pixel;
	}

	int vertexShader;
	int fragmentShader;
	int shaderProgram;

	ScreenBuffer();
	~ScreenBuffer();

	void Draw();
};