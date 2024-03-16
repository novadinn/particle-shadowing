#include "input.h"

std::unordered_map<SDL_Keycode, b8> Input::pressed_keys;
std::unordered_map<SDL_Keycode, b8> Input::released_keys;
std::unordered_map<SDL_Keycode, b8> Input::held_keys;
std::unordered_map<u8, b8> Input::pressed_mouse_buttons;
std::unordered_map<u8, b8> Input::released_mouse_buttons;
std::unordered_map<u8, b8> Input::held_mouse_buttons;
i32 Input::wheel_x;
i32 Input::wheel_y;

b8 Input::initialize() { return true; }

void Input::shutdown() {}

void Input::begin() {
  pressed_keys.clear();
  released_keys.clear();

  pressed_mouse_buttons.clear();
  released_mouse_buttons.clear();

  wheel_x = 0;
  wheel_y = 0;
}

void Input::keyDownEvent(const SDL_Event &event) {
  pressed_keys[event.key.keysym.sym] = true;
  held_keys[event.key.keysym.sym] = true;
}

void Input::keyUpEvent(const SDL_Event &event) {
  released_keys[event.key.keysym.sym] = true;
  held_keys[event.key.keysym.sym] = false;
}

void Input::mouseButtonDownEvent(const SDL_Event &event) {
  pressed_mouse_buttons[event.button.button] = true;
  held_mouse_buttons[event.button.button] = true;
}

void Input::mouseButtonUpEvent(const SDL_Event &event) {
  released_mouse_buttons[event.button.button] = true;
  held_mouse_buttons[event.button.button] = false;
}

void Input::wheelEvent(const SDL_Event &event) {
  wheel_x = event.wheel.x;
  wheel_y = event.wheel.y;
}

b8 Input::wasKeyPressed(SDL_Keycode key) { return pressed_keys[key]; }

b8 Input::wasKeyReleased(SDL_Keycode key) { return released_keys[key]; }

b8 Input::wasKeyHeld(SDL_Keycode key) { return held_keys[key]; }

b8 Input::wasMouseButtonPressed(u8 button) {
  return pressed_mouse_buttons[button];
}

b8 Input::wasMouseButtonReleased(u8 button) {
  return released_mouse_buttons[button];
}

b8 Input::wasMouseButtonHeld(u8 button) { return held_mouse_buttons[button]; }

void Input::getMousePosition(i32 *x, i32 *y) { SDL_GetMouseState(x, y); }