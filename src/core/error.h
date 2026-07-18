#ifndef ERROR_H
#define ERROR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Engine error codes.
typedef enum EngineError {
    ENGINE_ERROR_NONE,            ///< No error
    ENGINE_ERROR_INVALID_MODE,    ///< Invalid or unknown engine mode
    ENGINE_ERROR_INVALID_TIMEOUT, ///< Invalid timeout value
    ENGINE_ERROR_ALREADY_RUNNING, ///< Operation failed because engine is already running
    ENGINE_ERROR_NOT_RUNNING,     ///< Operation failed because engine is not running
    ENGINE_ERROR_CONFIG,          ///< Invalid engine configuration
    ENGINE_ERROR_CONTENT,         ///< Content provider returned empty or missing text
    ENGINE_ERROR_STATE,           ///< State machine error (invalid transition)
    ENGINE_ERROR_PROVIDER,        ///< Provider error (database query failure)
    ENGINE_ERROR_FILE,            ///< File I/O error (file not found, permission denied)
    ENGINE_ERROR_UNKNOWN          ///< Unknown or unspecified error
} EngineError;

/// @brief Convert an EngineError enum value to its human-readable string.
/// @param error      The EngineError to convert.
/// @param buffer     Output buffer for the string.
/// @param bufferSize Size of the output buffer.
void engineErrorToString(EngineError error, char* buffer, size_t bufferSize);

#ifdef __cplusplus
}
#endif

#endif // ERROR_H
