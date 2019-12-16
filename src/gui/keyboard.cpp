#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include "keyboard.h"
#include "window.h"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>

Gameboy * Keyboard::s_gb = nullptr;

void key_callback( GLFWwindow * window, int key, int scancode, int action, int mods ) {
	if ( action == GLFW_PRESS )
		Keyboard::RegisterKeyPress( key );
	else if ( action == GLFW_RELEASE )
		Keyboard::RegisterKeyRelease( key );

	ImGui_ImplGlfw_KeyCallback( window, key, scancode, action, mods );
}

void Keyboard::Init( Window & window ) {
	GLFWwindow * glWindow = window.GetGlfwWindow();

	glfwSetKeyCallback( glWindow, key_callback );
	glfwSetCharCallback( glWindow, ImGui_ImplGlfw_CharCallback );

	for ( int i = 0; i < MAX_CONCURRENT_KEY_DOWN; i++ ) {
		Keyboard::keyDowns[ i ] = KEY_NONE;
		Keyboard::keyPressed[ i ] = KEY_NONE;
	}
}

// Keys will be registered through callback,
// So we just want to flush key state
void Keyboard::Update() {
	for ( int & i : Keyboard::keyPressed )
		i = KEY_NONE;
}

void Keyboard::RegisterKeyPress( int key ) {
	if ( key == eKey::KEY_A ) {
		s_gb->mem.inputMask = BIT_UNSET( s_gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_A );
		s_gb->RaiseInterupt( 4 );
	}
	if ( key == eKey::KEY_S ) {
		s_gb->mem.inputMask = BIT_UNSET( s_gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_B );
		s_gb->RaiseInterupt( 4 );
	}
	if ( key == eKey::KEY_UP ) {
		s_gb->mem.inputMask = BIT_UNSET( s_gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_UP );
		s_gb->RaiseInterupt( 4 );
	}
	if ( key == eKey::KEY_DOWN ) {
		s_gb->mem.inputMask = BIT_UNSET( s_gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_DOWN );
		s_gb->RaiseInterupt( 4 );
	}
	if ( key == eKey::KEY_LEFT ) {
		s_gb->mem.inputMask = BIT_UNSET( s_gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_LEFT );
		s_gb->RaiseInterupt( 4 );
	}
	if ( key == eKey::KEY_RIGHT ) {
		s_gb->mem.inputMask = BIT_UNSET( s_gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_RIGHT );
		s_gb->RaiseInterupt( 4 );
	}
	if ( key == eKey::KEY_ENTER ) {
		s_gb->mem.inputMask = BIT_UNSET( s_gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_START );
		s_gb->RaiseInterupt( 4 );
	}
	if ( key == eKey::KEY_RIGHT_SHIFT ) {
		s_gb->mem.inputMask = BIT_UNSET( s_gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_SELECT );
		s_gb->RaiseInterupt( 4 );
	}
	for ( int & keyDown : Keyboard::keyDowns ) {
		if ( keyDown == KEY_NONE ) {
			keyDown = key;
			break;
		}
	}

	for ( int & i : Keyboard::keyPressed ) {
		if ( i == KEY_NONE ) {
			i = key;
			break;
		}
	}
}

void Keyboard::RegisterKeyRelease( int key ) {
	if ( key == eKey::KEY_A ) {
		s_gb->mem.inputMask = BIT_SET( s_gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_A );
	}
	if ( key == eKey::KEY_S ) {
		s_gb->mem.inputMask = BIT_SET( s_gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_B );
	}
	if ( key == eKey::KEY_UP ) {
		s_gb->mem.inputMask = BIT_SET( s_gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_UP );
	}
	if ( key == eKey::KEY_DOWN ) {
		s_gb->mem.inputMask = BIT_SET( s_gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_DOWN );
	}
	if ( key == eKey::KEY_LEFT ) {
		s_gb->mem.inputMask = BIT_SET( s_gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_LEFT );
	}
	if ( key == eKey::KEY_RIGHT ) {
		s_gb->mem.inputMask = BIT_SET( s_gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_RIGHT );
	}
	if ( key == eKey::KEY_ENTER ) {
		s_gb->mem.inputMask = BIT_SET( s_gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_START );
	}
	if ( key == eKey::KEY_RIGHT_SHIFT ) {
		s_gb->mem.inputMask = BIT_SET( s_gb->mem.inputMask, eGameBoyKeyValue::GB_KEY_SELECT );
	}
	for ( int & keyDown : Keyboard::keyDowns ) {
		if ( keyDown == key ) {
			// We could break here but at least we make sure to clean if a key
			// has been registered as pressed twice, and it doesnt cost much
			keyDown = KEY_NONE;
		}
	}
}

bool Keyboard::IsKeyDown( eKey key ) {
	for ( int keyDown : Keyboard::keyDowns ) {
		if ( keyDown == key ) {
			return true;
		}
	}
	return false;
}

// This is only true for the frame the key has been pressed
bool Keyboard::IsKeyPressed( eKey key ) {
	for ( int i : Keyboard::keyPressed ) {
		if ( i == key ) {
			return true;
		}
	}
	return false;
}

int Keyboard::keyDowns[ MAX_CONCURRENT_KEY_DOWN ] = {};
int Keyboard::keyPressed[ MAX_CONCURRENT_KEY_DOWN ] = {};
