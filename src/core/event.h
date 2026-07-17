#ifndef EVENT_H
#define EVENT_H

#include <stdint.h>
#include <stdio.h>

typedef struct Engine Engine;

/// @brief Engine events that can trigger registered callbacks.
typedef enum EngineEvent {
    ENGINE_EVENT_NONE,                ///< No event / default
    ENGINE_EVENT_STARTED,             ///< Engine started a session
    ENGINE_EVENT_STOPPED,             ///< Engine stopped (user, finish, or timeout)
    ENGINE_EVENT_PAUSED,              ///< Engine was paused
    ENGINE_EVENT_RESUMED,             ///< Engine was resumed from pause
    ENGINE_EVENT_TIMEOUT,             ///< Session exceeded configured timeout
    ENGINE_EVENT_FINISHED,            ///< Session completed (all text typed)
    ENGINE_EVENT_CORRECT_KEYSTROKE,   ///< A correct key was pressed
    ENGINE_EVENT_INCORRECT_KEYSTROKE, ///< An incorrect key was pressed
    ENGINE_EVENT_BACKSPACE,           ///< Backspace was pressed (flow mode)
    ENGINE_EVENT_SEGMENT_COMPLETED,   ///< A text segment/chunk was completed
    ENGINE_EVENT_ERROR                ///< An error occurred
} EngineEvent;

/// @brief Callback function type for engine events.
/// @param engine   The Engine instance that fired the event.
/// @param userData User-provided data registered with the callback.
typedef void (*EngineCallback)(Engine* engine, void* userData);

/// @brief Convert an EngineEvent enum value to its human-readable string.
/// @param event      The EngineEvent to convert.
/// @param buffer     Output buffer for the string.
/// @param bufferSize Size of the output buffer.
void engineEventToString(EngineEvent event, char* buffer, size_t bufferSize);

#endif
