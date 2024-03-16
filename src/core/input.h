#pragma once

#include "platform.h"

#include <SDL.h>
#include <unordered_map>

struct Input {
  static std::unordered_map<SDL_Keycode, b8> pressed_keys;
  static std::unordered_map<SDL_Keycode, b8> released_keys;
  static std::unordered_map<SDL_Keycode, b8> held_keys;

  static b8 initialize();
  static void shutdown();

  static void begin();
  static void keyDownEvent(const SDL_Event &event);
  static void keyUpEvent(const SDL_Event &event);

  static b8 wasKeyPressed(SDL_Keycode key);
  static b8 wasKeyReleased(SDL_Keycode key);
  static b8 wasKeyHeld(SDL_Keycode key);
};