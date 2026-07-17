#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/// @brief Maximum number of log files that can be added simultaneously.
#define LOGGER_MAX_FILES 4

/// @brief Log severity levels. Messages below the configured threshold are suppressed.
typedef enum LogLevel {
    LOG_LEVEL_DEBUG,   ///< Detailed diagnostic information
    LOG_LEVEL_INFO,    ///< General operational messages
    LOG_LEVEL_WARNING, ///< Non-critical issues that may need attention
    LOG_LEVEL_ERROR,   ///< Critical errors that may cause failures
    LOG_LEVEL_NONE     ///< Sentinel: suppress all messages when used as currentLevel
} LogLevel;

typedef struct Logger Logger;

/// @brief Create a new Logger instance.
/// @param level  Minimum log level; messages below this are suppressed.
/// @param enableStdout  Whether to write log messages to stdout.
/// @return A pointer to the newly created Logger, or NULL on allocation failure.
Logger* loggerCreate(LogLevel level, bool enableStdout);

/// @brief Destroy a Logger instance, closing any open log files.
/// @param logger The Logger to destroy (NULL is safe).
void loggerDestroy(Logger* logger);

/// @brief Set the minimum log level. Messages below this level are suppressed.
/// @param logger The Logger instance.
/// @param level  The new minimum log level.
void loggerSetLevel(Logger* logger, LogLevel level);

/// @brief Get the current minimum log level.
/// @param logger The Logger instance.
/// @return The current LogLevel, or LOG_LEVEL_NONE if logger is NULL.
LogLevel loggerGetLevel(Logger* logger);

/// @brief Enable or disable logging to stdout.
/// @param logger The Logger instance.
/// @param enable true to enable stdout output, false to disable.
void loggerLogToStdout(Logger* logger, bool enable);

/// @brief Add a file to receive log output. The file is opened in append mode.
/// @param logger   The Logger instance.
/// @param filepath Path to the log file.
/// @return true if the file was added successfully, false on failure or if
///         LOGGER_MAX_FILES have already been added.
bool loggerAddFile(Logger* logger, const char* filepath);

/// @brief Write a log message at the specified level.
/// @param logger  The Logger instance (NULL is safe).
/// @param level   The severity level of the message.
/// @param message The message string to log.
void loggerLog(Logger* logger, LogLevel level, const char* message);

#endif // LOGGER_H
