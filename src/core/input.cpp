#include "input.h"

std::unordered_map<SDL_Keycode, b8> Input::pressed_keys;
std::unordered_map<SDL_Keycode, b8> Input::released_keys;
std::unordered_map<SDL_Keycode, b8> Input::held_keys;

b8 Input::initialize() { return true; }

void Input::shutdown() {}

void Input::begin() {
  pressed_keys.clear();
  released_keys.clear();
}

void Input::keyDownEvent(const SDL_Event &event) {
  pressed_keys[event.key.keysym.sym] = true;
  held_keys[event.key.keysym.sym] = true;
}

void Input::keyUpEvent(const SDL_Event &event) {
  released_keys[event.key.keysym.sym] = true;
  held_keys[event.key.keysym.sym] = false;
}

b8 Input::wasKeyPressed(SDL_Keycode key) { return pressed_keys[key]; }

b8 Input::wasKeyReleased(SDL_Keycode key) { return released_keys[key]; }

b8 Input::wasKeyHeld(SDL_Keycode key) { return held_keys[key]; }