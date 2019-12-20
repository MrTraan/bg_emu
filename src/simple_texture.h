#pragma once
#include <glad/glad.h>
#include "gb_emu.h"

struct SimpleTexture {
	Pixel * buffer = nullptr;
	int		width = 0;
	int		height = 0;
	uint32	textureHandler = 0;

	void Allocate( int width, int height ) {
		this->width = width;
		this->height = height;
		buffer = new Pixel[ width * height ];
		glGenTextures( 1, &textureHandler );
		glBindTexture( GL_TEXTURE_2D, textureHandler );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	}

	void Destroy() { delete[] buffer; }

	void Bind() { glBindTexture( GL_TEXTURE_2D, textureHandler ); }

	void Update() {
		Bind();
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer );
	}

	void SetPixel( const Pixel & pixel, int x, int y ) {
		gbemu_assert( x < width );
		gbemu_assert( y < height );
		buffer[ x + y * width ] = pixel;
	}

	void Clear() { memset( buffer, 0, width * height * sizeof( Pixel ) ); }
};
