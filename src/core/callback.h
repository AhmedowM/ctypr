#ifndef CALLBACK_H
#define CALLBACK_H

#include "event.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Engine Engine;

/// @brief Register a callback for ENGINE_EVENT_STARTED.
/// @param engine   Engine instance.
/// @param callback Function to call on event.
/// @param userData User data passed to callback.
/// @return Slot index on success, -1 if full.
int engineOnStarted(Engine* engine, EngineCallback callback, void* userData);

/// @brief Register a callback for ENGINE_EVENT_STOPPED.
/// @param engine   Engine instance.
/// @param callback Function to call on event.
/// @param userData User data passed to callback.
/// @return Slot index on success, -1 if full.
int engineOnStopped(Engine* engine, EngineCallback callback, void* userData);

/// @brief Register a callback for ENGINE_EVENT_PAUSED.
/// @param engine   Engine instance.
/// @param callback Function to call on event.
/// @param userData User data passed to callback.
/// @return Slot index on success, -1 if full.
int engineOnPaused(Engine* engine, EngineCallback callback, void* userData);

/// @brief Register a callback for ENGINE_EVENT_RESUMED.
/// @param engine   Engine instance.
/// @param callback Function to call on event.
/// @param userData User data passed to callback.
/// @return Slot index on success, -1 if full.
int engineOnResumed(Engine* engine, EngineCallback callback, void* userData);

/// @brief Register a callback for ENGINE_EVENT_TIMEOUT.
/// @param engine   Engine instance.
/// @param callback Function to call on event.
/// @param userData User data passed to callback.
/// @return Slot index on success, -1 if full.
int engineOnTimeout(Engine* engine, EngineCallback callback, void* userData);

/// @brief Register a callback for ENGINE_EVENT_FINISHED.
/// @param engine   Engine instance.
/// @param callback Function to call on event.
/// @param userData User data passed to callback.
/// @return Slot index on success, -1 if full.
int engineOnFinished(Engine* engine, EngineCallback callback, void* userData);

/// @brief Register a callback for ENGINE_EVENT_CORRECT_KEYSTROKE.
/// @param engine   Engine instance.
/// @param callback Function to call on event.
/// @param userData User data passed to callback.
/// @return Slot index on success, -1 if full.
int engineOnCorrectKeystroke(Engine* engine, EngineCallback callback, void* userData);

/// @brief Register a callback for ENGINE_EVENT_INCORRECT_KEYSTROKE.
/// @param engine   Engine instance.
/// @param callback Function to call on event.
/// @param userData User data passed to callback.
/// @return Slot index on success, -1 if full.
int engineOnIncorrectKeystroke(Engine* engine, EngineCallback callback, void* userData);

/// @brief Register a callback for ENGINE_EVENT_BACKSPACE.
/// @param engine   Engine instance.
/// @param callback Function to call on event.
/// @param userData User data passed to callback.
/// @return Slot index on success, -1 if full.
int engineOnBackspace(Engine* engine, EngineCallback callback, void* userData);

/// @brief Register a callback for ENGINE_EVENT_SEGMENT_COMPLETED.
/// @param engine   Engine instance.
/// @param callback Function to call on event.
/// @param userData User data passed to callback.
/// @return Slot index on success, -1 if full.
int engineOnSegmentCompleted(Engine* engine, EngineCallback callback, void* userData);

/// @brief Register a callback for ENGINE_EVENT_ERROR.
/// @param engine   Engine instance.
/// @param callback Function to call on event.
/// @param userData User data passed to callback.
/// @return Slot index on success, -1 if full.
int engineOnError(Engine* engine, EngineCallback callback, void* userData);

/// @brief Remove a registered callback.
/// @param engine Engine instance.
/// @param event  Event type the callback was registered on.
/// @param slotId Slot index returned by the registration function.
void engineDisconnect(Engine* engine, EngineEvent event, int slotId);

/// @brief Remove all callbacks for a specific event.
/// @param engine Engine instance.
/// @param event  Event type to clear.
void engineClearEvent(Engine* engine, EngineEvent event);

#ifdef __cplusplus
}
#endif

#endif // CALLBACK_H