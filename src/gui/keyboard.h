#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "../gb_emu.h"
#include "../memory.h"
#include "../cpu.h"

class Window;

#define MAX_CONCURRENT_KEY_DOWN 8

enum eKey {
	KEY_NONE = 0,
	KEY_W = GLFW_KEY_W,
	KEY_A = GLFW_KEY_A,
	KEY_S = GLFW_KEY_S,
	KEY_D = GLFW_KEY_D,
	KEY_V = GLFW_KEY_V,

	KEY_UP = GLFW_KEY_UP,
	KEY_DOWN = GLFW_KEY_DOWN,
	KEY_LEFT = GLFW_KEY_LEFT,
	KEY_RIGHT = GLFW_KEY_RIGHT,

	KEY_LEFT_SHIFT = GLFW_KEY_LEFT_SHIFT,
	KEY_RIGHT_SHIFT = GLFW_KEY_RIGHT_SHIFT,
	KEY_ESCAPE = GLFW_KEY_ESCAPE,
	KEY_SPACE = GLFW_KEY_SPACE,
	KEY_ENTER = GLFW_KEY_ENTER
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

class Keyboard {
   public:
	static void Init(Window& window);
	static void Update();

	static void RegisterKeyPress(int key);
	static void RegisterKeyRelease(int key);

	static bool IsKeyDown(eKey key);
	static bool IsKeyPressed(eKey key);
	
	static Memory * s_mem;
	static Cpu * s_cpu;

   private:
	static int keyDowns[MAX_CONCURRENT_KEY_DOWN];
	static int keyPressed[MAX_CONCURRENT_KEY_DOWN];
};
