#ifndef EVENT_H
#define EVENT_H

#include "callback.h"

#include <stdint.h>
#include <stdio.h>

typedef enum EngineEvent {
    ENGINE_EVENT_NONE,
    ENGINE_EVENT_STARTED,
    ENGINE_EVENT_STOPPED,
    ENGINE_EVENT_PAUSED,
    ENGINE_EVENT_RESUMED,
    ENGINE_EVENT_TIMEOUT,
    ENGINE_EVENT_FINISHED,
    ENGINE_EVENT_CORRECT_KEYSTROKE,
    ENGINE_EVENT_INCORRECT_KEYSTROKE,
    ENGINE_EVENT_BACKSPACE,
    ENGINE_EVENT_SEGMENT_COMPLETED,
    ENGINE_EVENT_ERROR
} EngineEvent;

typedef struct Engine Engine; // Forward declaration of Engine struct

/// @brief Convert an EngineEvent to a string representation
/// @param event The EngineEvent to convert
/// @param buffer The buffer to store the string representation
/// @param bufferSize The size of the buffer
void engineEventToString(EngineEvent event, char* buffer, size_t bufferSize);

/// @brief Handle an engine event
/// @param engine The Engine instance
/// @param event The EngineEvent to handle
/// @param callback The callback function to execute for the event
/// @param userData User-defined data to pass to the callback function
void onEngineEvent(Engine* engine, EngineEvent event, EngineCallback callback, void* userData);

#endif // EVENT_H