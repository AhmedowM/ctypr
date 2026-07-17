#ifndef CALLBACK_H
#define CALLBACK_H

#include "event.h"

#include <stdbool.h>

#define MAX_CALLBACKS 10

typedef struct Engine Engine; // Forward declaration of Engine struct

/// @brief Register a callback function for engine events
/// @param engine The Engine instance
/// @param event The event to register the callback for
/// @param callback The callback function to register
/// @param userData User-defined data to pass to the callback function
bool engineRegisterCallback(Engine* engine, EngineEvent* event, EngineCallback callback, void* userData);

/// @brief Execute all registered callbacks
/// @param engine The Engine instance
/// @param event The event to trigger the callbacks for
void engineExecuteCallbacks(Engine* engine, EngineEvent* event);

#endif // CALLBACK_H