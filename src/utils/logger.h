#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define LOGGER_MAX_FILES 4

typedef enum LogLevel {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_NONE
} LogLevel;

typedef struct Logger Logger;

Logger* loggerCreate(LogLevel level, bool enableStdout);
void loggerDestroy(Logger* logger);

void loggerSetLevel(Logger* logger, LogLevel level);
LogLevel loggerGetLevel(Logger* logger);

void loggerLogToStdout(Logger* logger, bool enable);

bool loggerAddFile(Logger* logger, const char* filepath);

void loggerLog(Logger* logger, LogLevel level, const char* message);

#endif // LOGGER_H