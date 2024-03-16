#pragma once

#include "core/platform.h"

#include <cstring>
#include <string>

#ifdef PLATFORM_WINDOWS
#define SLASH_CH '\\'
#else
#define SLASH_CH '/'
#endif

struct FileSystem {
  static std::string joinPath(const char *path) {
    std::string result;
    for (u32 i = 0; i < strlen(path); ++i) {
      if (path[i] == '/' || path[i] == '\\') {
        result += SLASH_CH;
        continue;
      }

      result += path[i];
    }

    return result;
  }
};