#pragma once

enum LogLevel {
  LOG_LEVEL_FATAL,
  LOG_LEVEL_ERROR,
  LOG_LEVEL_WARN,
  LOG_LEVEL_INFO,
  LOG_LEVEL_DEBUG,
  LOG_LEVEL_TRACE,
};

struct Logger {
  static void logOutput(LogLevel level, const char *message, ...);
};

#define FATAL(message, ...)                                                    \
  Logger::logOutput(LOG_LEVEL_FATAL, message, ##__VA_ARGS__);
#define ERROR(message, ...)                                                    \
  Logger::logOutput(LOG_LEVEL_ERROR, message, ##__VA_ARGS__);
#define WARN(message, ...)                                                     \
  Logger::logOutput(LOG_LEVEL_WARN, message, ##__VA_ARGS__);
#define INFO(message, ...)                                                     \
  Logger::logOutput(LOG_LEVEL_INFO, message, ##__VA_ARGS__);

#ifndef NDEBUG
#define DEBUG(message, ...)                                                    \
  Logger::logOutput(LOG_LEVEL_DEBUG, message, ##__VA_ARGS__);
#define TRACE(message, ...)                                                    \
  Logger::logOutput(LOG_LEVEL_TRACE, message, ##__VA_ARGS__);
#else
#define DEBUG(message, ...)
#define TRACE(message, ...)
#endif