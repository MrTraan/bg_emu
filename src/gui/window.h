#pragma once
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <stdexcept>
#include "keyboard.h"
#include "screen_buffer.h"
#include "../ppu.h"

static void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

static void glfwErrorCallback(int code, const char* msg) {
	printf("Glfw error: %s\n", msg);
}

struct Window {
public:
	char* Title;
	int Width;
	int Height;

	void Allocate(int width, int height, char* title) {
		this->Width = width;
		this->Height = height;
		this->Title = title;

		if (!glfwInit()) {
			throw std::runtime_error("Fatal Error: Could not instantiate glfw");
		}
		glfwSetErrorCallback(glfwErrorCallback);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		//		glfwWindowHint(GLFW_SAMPLES, 4);

		auto monitor = glfwGetPrimaryMonitor();
		auto videoMode = glfwGetVideoMode(monitor);
		this->glWindow = glfwCreateWindow(this->Width, this->Height,
			this->Title, NULL, NULL);
		if (!this->glWindow) {
			throw std::runtime_error(
				"Fatal Error: Could not create GLFW Window");
		}

		glfwMakeContextCurrent(this->glWindow);
		glfwSetFramebufferSizeCallback(this->glWindow, framebufferSizeCallback);
		glfwSwapInterval(1);

		// glad: load all OpenGL function pointers
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
			throw std::runtime_error("Failed to initialize glad\n");


		// configure global opengl state
		glEnable(GL_DEPTH_TEST);
	}

	void Destroy() {
		glfwDestroyWindow(glWindow);
		glfwTerminate();
	}

	bool ShouldClose() {
		return glfwWindowShouldClose(this->glWindow) != 0;
	}

	void Clear() {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void SwapBuffers() {
		glfwSwapBuffers(this->glWindow);
	}

	void PollEvents() {
		//@HARDCODED
		Keyboard::Update();
		glfwPollEvents();
	}

	void ProcessInput() {
		if (Keyboard::IsKeyDown(KEY_ESCAPE))
			glfwSetWindowShouldClose(this->glWindow, GL_TRUE);
	}

	// I added a getter on this member because it should only be queried for
	// keyboard setup
	GLFWwindow* GetGlfwWindow() {
		return this->glWindow;
	}

private:
	GLFWwindow* glWindow;
};
