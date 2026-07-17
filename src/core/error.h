#ifndef ERROR_H
#define ERROR_H

#include <stdint.h>

typedef enum EngineError {
    ENGINE_ERROR_NONE,
    ENGINE_ERROR_INVALID_MODE,
    ENGINE_ERROR_INVALID_TIMEOUT,
    ENGINE_ERROR_ALREADY_RUNNING,
    ENGINE_ERROR_NOT_RUNNING,
    ENGINE_ERROR_UNKNOWN
} EngineError;

void engineErrorToString(EngineError error, char* buffer, size_t bufferSize);

#endif // ERROR_H