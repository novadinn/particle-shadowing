#pragma once

#include "platform.h"

#include <SDL.h>
#include <unordered_map>

struct Input {
  static std::unordered_map<SDL_Keycode, b8> pressed_keys;
  static std::unordered_map<SDL_Keycode, b8> released_keys;
  static std::unordered_map<SDL_Keycode, b8> held_keys;
  static std::unordered_map<u8, b8> pressed_mouse_buttons;
  static std::unordered_map<u8, b8> released_mouse_buttons;
  static std::unordered_map<u8, b8> held_mouse_buttons;
  static i32 wheel_x;
  static i32 wheel_y;

  static b8 initialize();
  static void shutdown();

  static void begin();
  static void keyDownEvent(const SDL_Event &event);
  static void keyUpEvent(const SDL_Event &event);

  static void mouseButtonDownEvent(const SDL_Event &event);
  static void mouseButtonUpEvent(const SDL_Event &event);

  static void wheelEvent(const SDL_Event &event);

  static b8 wasKeyPressed(SDL_Keycode key);
  static b8 wasKeyReleased(SDL_Keycode key);
  static b8 wasKeyHeld(SDL_Keycode key);

  static b8 wasMouseButtonPressed(u8 button);
  static b8 wasMouseButtonReleased(u8 button);
  static b8 wasMouseButtonHeld(u8 button);

  static void getMousePosition(i32 *x, i32 *y);
};