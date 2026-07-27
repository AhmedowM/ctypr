#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Maximum log files simultaneously.
#define LOGGER_MAX_FILES 4

/// @brief Log severity levels. Messages below threshold are suppressed.
typedef enum LogLevel {
    LOG_LEVEL_DEBUG,   ///< Detailed diagnostic information
    LOG_LEVEL_INFO,    ///< General operational messages
    LOG_LEVEL_WARNING, ///< Non-critical issues needing attention
    LOG_LEVEL_ERROR,   ///< Critical errors that may cause failures
    LOG_LEVEL_NONE     ///< Sentinel: suppress all messages as threshold
} LogLevel;

typedef struct Logger Logger;

/// @brief Create a new Logger.
/// @param level        Minimum log level; messages below are suppressed.
/// @param enableStdout Whether to write to stdout.
/// @return New Logger, or NULL on allocation failure.
Logger* loggerCreate(LogLevel level, bool enableStdout);

/// @brief Destroy a Logger, closing any open files.
/// @param logger Logger to destroy (NULL is safe).
void loggerDestroy(Logger* logger);

/// @brief Set minimum log level.
/// @param logger Logger instance.
/// @param level  New minimum level.
void loggerSetLevel(Logger* logger, LogLevel level);

/// @brief Get current minimum log level.
/// @param logger Logger instance.
/// @return Current LogLevel, or LOG_LEVEL_NONE if logger is NULL.
LogLevel loggerGetLevel(Logger* logger);

/// @brief Write a log message at specified level.
/// @param logger Logger instance (NULL is safe).
/// @param level  Message severity.
/// @param message Message string to log.
void loggerLog(Logger* logger, LogLevel level, const char* message);

/// @brief Add a file for log output (append mode).
/// @param logger   Logger instance.
/// @param filepath Log file path.
/// @return true if added, false on failure or if LOGGER_MAX_FILES reached.
bool loggerAddFile(Logger* logger, const char* filepath);

/// @brief Enable or disable stdout logging.
/// @param logger Logger instance.
/// @param enable true to enable, false to disable.
void loggerLogToStdout(Logger* logger, bool enable);

#ifdef __cplusplus
}
#endif

#endif // LOGGER_H