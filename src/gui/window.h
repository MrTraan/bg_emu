#pragma once
#include <glad/glad.h>
#include <SDL.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"
#include <stdexcept>
#include "../gb_emu.h"
#include "../gameboy.h"

enum eKey {
	KEY_NONE = 0,
	KEY_W = SDLK_w,
	KEY_A = SDLK_a,
	KEY_S = SDLK_s,
	KEY_D = SDLK_d,
	KEY_V = SDLK_v,

	KEY_UP = SDLK_UP,
	KEY_DOWN = SDLK_DOWN,
	KEY_LEFT = SDLK_LEFT,
	KEY_RIGHT = SDLK_RIGHT,

	KEY_LEFT_SHIFT = SDLK_LSHIFT,
	KEY_RIGHT_SHIFT = SDLK_RSHIFT,
	KEY_ESCAPE = SDLK_ESCAPE,
	KEY_SPACE = SDLK_SPACE,
	KEY_ENTER = SDLK_RETURN,
};

enum eGameBoyKeyValue : byte {
	GB_KEY_A = 0,
	GB_KEY_B = 1,
	GB_KEY_SELECT = 2,
	GB_KEY_START = 3,

	GB_KEY_RIGHT = 4,
	GB_KEY_LEFT = 5,
	GB_KEY_UP = 6,
	GB_KEY_DOWN = 7,
};

struct Window
{
public:
	const char *Title;
	int Width;
	int Height;

	void Allocate(int width, int height, const char *title)
	{
		this->Width = width;
		this->Height = height;
		this->Title = title;

#if __APPLE__
		// GL 3.2 Core + GLSL 150
		const char *glsl_version = "#version 150";
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#else
		// GL 3.0 + GLSL 130
		const char *glsl_version = "#version 130";
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif

		SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
		glWindow = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, window_flags);
		if (!this->glWindow)
		{
			throw std::runtime_error("Fatal Error: Could not create GLFW Window");
		}
		glContext = SDL_GL_CreateContext(glWindow);
		SDL_GL_MakeCurrent(glWindow, glContext);
		SDL_GL_SetSwapInterval(1); // Enable vsync

		// glad: load all OpenGL function pointers
		if (!gladLoadGL())
			throw std::runtime_error("Failed to initialize glad\n");

		// configure global opengl state
		glEnable(GL_DEPTH_TEST);
	}

	void Destroy()
	{
		SDL_GL_DeleteContext(glContext);
		SDL_DestroyWindow(glWindow);
	}

	bool ShouldClose() { return shouldClose; }

	void Clear() { glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }

	void SwapBuffers() { SDL_GL_SwapWindow(this->glWindow); }

	void PollEvents(Gameboy *gb)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
				shouldClose = true;
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(glWindow))
				shouldClose = true;
			if (event.type == SDL_DROPFILE) {
				char * cartridgePath = event.drop.file;
				gb->LoadCart( cartridgePath );
				SDL_free(cartridgePath);
			}
			if (event.type == SDL_WINDOWEVENT_RESIZED) {
				Width = event.window.data1;
				Height = event.window.data1;
				gb->ppu.frontBuffer.RefreshSize( *this );
				gb->ppu.backBuffer.RefreshSize( *this );
			}
			if (event.type == SDL_KEYUP)
			{
				auto key = event.key.keysym.sym;
				if (key == eKey::KEY_A)
				{
					gb->mem.inputMask = BIT_UNSET(gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_A);
					gb->RaiseInterupt(4);
				}
				if (key == eKey::KEY_S)
				{
					gb->mem.inputMask = BIT_UNSET(gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_B);
					gb->RaiseInterupt(4);
				}
				if (key == eKey::KEY_UP)
				{
					gb->mem.inputMask = BIT_UNSET(gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_UP);
					gb->RaiseInterupt(4);
				}
				if (key == eKey::KEY_DOWN)
				{
					gb->mem.inputMask = BIT_UNSET(gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_DOWN);
					gb->RaiseInterupt(4);
				}
				if (key == eKey::KEY_LEFT)
				{
					gb->mem.inputMask = BIT_UNSET(gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_LEFT);
					gb->RaiseInterupt(4);
				}
				if (key == eKey::KEY_RIGHT)
				{
					gb->mem.inputMask = BIT_UNSET(gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_RIGHT);
					gb->RaiseInterupt(4);
				}
				if (key == eKey::KEY_ENTER)
				{
					gb->mem.inputMask = BIT_UNSET(gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_START);
					gb->RaiseInterupt(4);
				}
				if (key == eKey::KEY_RIGHT_SHIFT)
				{
					gb->mem.inputMask = BIT_UNSET(gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_SELECT);
					gb->RaiseInterupt(4);
				}
			}
			if (event.type == SDL_KEYDOWN)
			{
				auto key = event.key.keysym.sym;
				if (key == eKey::KEY_A)
				{
					gb->mem.inputMask = BIT_SET(gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_A);
				}
				if (key == eKey::KEY_S)
				{
					gb->mem.inputMask = BIT_SET(gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_B);
				}
				if (key == eKey::KEY_UP)
				{
					gb->mem.inputMask = BIT_SET(gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_UP);
				}
				if (key == eKey::KEY_DOWN)
				{
					gb->mem.inputMask = BIT_SET(gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_DOWN);
				}
				if (key == eKey::KEY_LEFT)
				{
					gb->mem.inputMask = BIT_SET(gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_LEFT);
				}
				if (key == eKey::KEY_RIGHT)
				{
					gb->mem.inputMask = BIT_SET(gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_RIGHT);
				}
				if (key == eKey::KEY_ENTER)
				{
					gb->mem.inputMask = BIT_SET(gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_START);
				}
				if (key == eKey::KEY_RIGHT_SHIFT)
				{
					gb->mem.inputMask = BIT_SET(gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_SELECT);
				}
			}
		}
	}

	SDL_Window *glWindow;
	SDL_GLContext glContext;
	bool shouldClose = false;
};
