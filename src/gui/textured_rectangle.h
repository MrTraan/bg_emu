#pragma once

#include <string.h>
#include "../simple_texture.h"

struct Window;

struct TexturedRectangle {
	SimpleTexture	texture;
	uint32			VBO;
	uint32			VAO;
	uint32			EBO;
	int				shaderProgram;
	int				width = 0;
	int				height = 0;
	int				x = 0;
	int				y = 0;

	float					vertices[ 3 * 4 + 2 * 4 ]; // 4 times 3 float coords + 4 times 2 float texture coords
	static constexpr uint32 indices[ 6 ] = { 0, 2, 1, 0, 3, 2 };

	void Allocate( int x, int y, int width, int height, const Window & window );
	void Destroy();
	void Draw();
	void Resize( int x, int y, int width, int height, const Window & window );
	void RefreshSize( const Window & window );
};