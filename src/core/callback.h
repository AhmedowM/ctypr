#ifndef CALLBACK_H
#define CALLBACK_H

#include "event.h"
#include "signal.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct Engine Engine;

/// @brief Register a callback for the ENGINE_EVENT_STARTED event.
/// @param engine   The Engine instance.
/// @param callback The callback function to register.
/// @param userData User-defined data passed to the callback when invoked.
/// @return Slot index (0-based) on success, or -1 if the signal slot list is full.
int engineOnStarted(Engine* engine, EngineCallback callback, void* userData);

/// @brief Register a callback for the ENGINE_EVENT_STOPPED event.
/// @param engine   The Engine instance.
/// @param callback The callback function to register.
/// @param userData User-defined data passed to the callback when invoked.
/// @return Slot index on success, or -1 if the signal slot list is full.
int engineOnStopped(Engine* engine, EngineCallback callback, void* userData);

/// @brief Register a callback for the ENGINE_EVENT_PAUSED event.
/// @param engine   The Engine instance.
/// @param callback The callback function to register.
/// @param userData User-defined data passed to the callback when invoked.
/// @return Slot index on success, or -1 if the signal slot list is full.
int engineOnPaused(Engine* engine, EngineCallback callback, void* userData);

/// @brief Register a callback for the ENGINE_EVENT_RESUMED event.
/// @param engine   The Engine instance.
/// @param callback The callback function to register.
/// @param userData User-defined data passed to the callback when invoked.
/// @return Slot index on success, or -1 if the signal slot list is full.
int engineOnResumed(Engine* engine, EngineCallback callback, void* userData);

/// @brief Register a callback for the ENGINE_EVENT_TIMEOUT event.
/// @param engine   The Engine instance.
/// @param callback The callback function to register.
/// @param userData User-defined data passed to the callback when invoked.
/// @return Slot index on success, or -1 if the signal slot list is full.
int engineOnTimeout(Engine* engine, EngineCallback callback, void* userData);

/// @brief Register a callback for the ENGINE_EVENT_FINISHED event.
/// @param engine   The Engine instance.
/// @param callback The callback function to register.
/// @param userData User-defined data passed to the callback when invoked.
/// @return Slot index on success, or -1 if the signal slot list is full.
int engineOnFinished(Engine* engine, EngineCallback callback, void* userData);

/// @brief Register a callback for the ENGINE_EVENT_CORRECT_KEYSTROKE event.
/// @param engine   The Engine instance.
/// @param callback The callback function to register.
/// @param userData User-defined data passed to the callback when invoked.
/// @return Slot index on success, or -1 if the signal slot list is full.
int engineOnCorrectKeystroke(Engine* engine, EngineCallback callback, void* userData);

/// @brief Register a callback for the ENGINE_EVENT_INCORRECT_KEYSTROKE event.
/// @param engine   The Engine instance.
/// @param callback The callback function to register.
/// @param userData User-defined data passed to the callback when invoked.
/// @return Slot index on success, or -1 if the signal slot list is full.
int engineOnIncorrectKeystroke(Engine* engine, EngineCallback callback, void* userData);

/// @brief Register a callback for the ENGINE_EVENT_BACKSPACE event.
/// @param engine   The Engine instance.
/// @param callback The callback function to register.
/// @param userData User-defined data passed to the callback when invoked.
/// @return Slot index on success, or -1 if the signal slot list is full.
int engineOnBackspace(Engine* engine, EngineCallback callback, void* userData);

/// @brief Register a callback for the ENGINE_EVENT_SEGMENT_COMPLETED event.
/// @param engine   The Engine instance.
/// @param callback The callback function to register.
/// @param userData User-defined data passed to the callback when invoked.
/// @return Slot index on success, or -1 if the signal slot list is full.
int engineOnSegmentCompleted(Engine* engine, EngineCallback callback, void* userData);

/// @brief Register a callback for the ENGINE_EVENT_ERROR event.
/// @param engine   The Engine instance.
/// @param callback The callback function to register.
/// @param userData User-defined data passed to the callback when invoked.
/// @return Slot index on success, or -1 if the signal slot list is full.
int engineOnError(Engine* engine, EngineCallback callback, void* userData);

/// @brief Remove a previously registered callback by event type and slot index.
/// @param engine The Engine instance.
/// @param event  The event type the callback was registered on.
/// @param slotId The slot index returned by the registration function.
void engineDisconnect(Engine* engine, EngineEvent event, int slotId);

/// @brief Remove all registered callbacks for a specific event type.
/// @param engine The Engine instance.
/// @param event  The event type to clear.
void engineClearEvent(Engine* engine, EngineEvent event);

#endif
