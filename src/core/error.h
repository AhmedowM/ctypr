#ifndef ERROR_H
#define ERROR_H

#include <stdint.h>

/// @brief Engine error codes.
typedef enum EngineError {
    ENGINE_ERROR_NONE,            ///< No error
    ENGINE_ERROR_INVALID_MODE,    ///< Invalid or unknown engine mode
    ENGINE_ERROR_INVALID_TIMEOUT, ///< Invalid timeout value
    ENGINE_ERROR_ALREADY_RUNNING, ///< Operation failed because engine is already running
    ENGINE_ERROR_NOT_RUNNING,     ///< Operation failed because engine is not running
    ENGINE_ERROR_UNKNOWN          ///< Unknown or unspecified error
} EngineError;

/// @brief Convert an EngineError enum value to its human-readable string.
/// @param error      The EngineError to convert.
/// @param buffer     Output buffer for the string.
/// @param bufferSize Size of the output buffer.
void engineErrorToString(EngineError error, char* buffer, size_t bufferSize);

#endif // ERROR_H
