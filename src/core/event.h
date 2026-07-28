#ifndef EVENT_H
#define EVENT_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Engine Engine;

/// @brief Engine events that can trigger callbacks.
typedef enum EngineEvent {
    ENGINE_EVENT_NONE,                ///< No event / default
    ENGINE_EVENT_STARTED,             ///< Session started
    ENGINE_EVENT_STOPPED,             ///< Session stopped (user, finish, or timeout)
    ENGINE_EVENT_PAUSED,              ///< Session paused
    ENGINE_EVENT_RESUMED,             ///< Session resumed
    ENGINE_EVENT_TIMEOUT,             ///< Session timed out
    ENGINE_EVENT_FINISHED,            ///< Session completed (all text typed)
    ENGINE_EVENT_CORRECT_KEYSTROKE,   ///< Correct key pressed
    ENGINE_EVENT_INCORRECT_KEYSTROKE, ///< Incorrect key pressed
    ENGINE_EVENT_BACKSPACE,           ///< Backspace pressed (FlowMode)
    ENGINE_EVENT_SEGMENT_COMPLETED,   ///< Text segment/chunk completed
    ENGINE_EVENT_ERROR                ///< Error occurred
} EngineEvent;

/// @brief Callback function type for engine events.
/// @param engine   Engine instance that fired the event.
/// @param userData User data registered with the callback.
typedef void (*EngineCallback)(Engine* engine, void* userData);

/// @brief Convert an EngineEvent to its string representation.
/// @param event      Event to convert.
/// @param buffer     Output buffer.
/// @param bufferSize Buffer size.
void engineEventToString(EngineEvent event, char* buffer, size_t bufferSize);

#ifdef __cplusplus
}
#endif

#endif // EVENT_H