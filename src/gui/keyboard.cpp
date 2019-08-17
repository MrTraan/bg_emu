#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include "keyboard.h"
#include "window.h"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>

void key_callback(GLFWwindow* window,
                  int key,
                  int scancode,
                  int action,
                  int mods) {
	if (action == GLFW_PRESS)
		Keyboard::RegisterKeyPress(key);
	else if (action == GLFW_RELEASE)
		Keyboard::RegisterKeyRelease(key);
	
	ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
}

void Keyboard::Init(Window& window) {
	GLFWwindow* glWindow = window.GetGlfwWindow();

	glfwSetKeyCallback(glWindow, key_callback);
	glfwSetCharCallback(glWindow, ImGui_ImplGlfw_CharCallback);

	for (int i = 0; i < MAX_CONCURRENT_KEY_DOWN; i++) {
		Keyboard::keyDowns[i] = KEY_NONE;
		Keyboard::keyPressed[i] = KEY_NONE;
	}
}

// Keys will be registered through callback,
// So we just want to flush key state
void Keyboard::Update() {
	for (int &i : Keyboard::keyPressed)
		i = KEY_NONE;
}

void Keyboard::RegisterKeyPress(int key) {
	for (int &keyDown : Keyboard::keyDowns) {
		if (keyDown == KEY_NONE) {
			keyDown = key;
			break;
		}
	}

	for (int &i : Keyboard::keyPressed) {
		if (i == KEY_NONE) {
			i = key;
			break;
		}
	}
}

void Keyboard::RegisterKeyRelease(int key) {
	for (int &keyDown : Keyboard::keyDowns) {
		if (keyDown == key) {
			// We could break here but at least we make sure to clean if a key
			// has been registered as pressed twice, and it doesnt cost much
			keyDown = KEY_NONE;
		}
	}
}

bool Keyboard::IsKeyDown(eKey key) {
	for (int keyDown : Keyboard::keyDowns) {
		if (keyDown == key) {
			return true;
		}
	}
	return false;
}

// This is only true for the frame the key has been pressed
bool Keyboard::IsKeyPressed(eKey key) {
	for (int i : Keyboard::keyPressed) {
		if (i == key) {
			return true;
		}
	}
	return false;
}

int Keyboard::keyDowns[MAX_CONCURRENT_KEY_DOWN] = {};
int Keyboard::keyPressed[MAX_CONCURRENT_KEY_DOWN] = {};
