#ifndef CALLBACK_H
#define CALLBACK_H

#include "event.h"

#include <stdbool.h>

/// @brief Maximum number of callbacks that can be registered across all event types.
#define MAX_CALLBACKS 10

typedef struct Engine Engine;

/// @brief Register a callback function to be invoked on a specific engine event.
/// @param engine   The Engine instance.
/// @param event    Pointer to the EngineEvent to listen for. If NULL, defaults to ENGINE_EVENT_NONE.
/// @param callback The callback function to register.
/// @param userData User-defined data passed to the callback when invoked.
/// @return true if the callback was registered, false if the callback table is full.
bool engineRegisterCallback(Engine* engine, EngineEvent* event, EngineCallback callback, void* userData);

/// @brief Execute all registered callbacks that match the given event.
/// @param engine The Engine instance.
/// @param event  Pointer to the EngineEvent to trigger callbacks for.
void engineExecuteCallbacks(Engine* engine, EngineEvent* event);

#endif // CALLBACK_H
