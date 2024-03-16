#pragma once

#include <assert.h>

/* TODO: get rid of that macro and handle errors by our own */
#define VK_CHECK(result)                                                       \
  { assert(result == VK_SUCCESS); }
