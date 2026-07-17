#ifndef CALLBACK_H
#define CALLBACK_H

// #include "event.h"
#include <stdbool.h>

#define MAX_CALLBACKS 10

typedef struct Engine Engine; // Forward declaration of Engine struct

typedef void (*EngineCallback)(Engine* engine, void* userData); // Callback function type for engine events

/// @brief Register a callback function for engine events
/// @param engine The Engine instance
/// @param event The event to register the callback for
/// @param callback The callback function to register
/// @param userData User-defined data to pass to the callback function
bool engineRegisterCallback(Engine* engine, void* event, EngineCallback callback, void* userData);

/// @brief Execute all registered callbacks
/// @param engine The Engine instance
/// @param event The event to trigger the callbacks for
void engineExecuteCallbacks(Engine* engine, void* event);

#endif // CALLBACK_H