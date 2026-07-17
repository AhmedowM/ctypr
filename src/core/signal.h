#ifndef SIGNAL_H
#define SIGNAL_H

#include "event.h"

#include <stdbool.h>
#include <stdint.h>

/// @brief Opaque per-event callback list. Each event type in the engine owns one Signal.
///        The internal slot array is fixed-size (see signal_internal.h).
typedef struct Signal Signal;

/// @brief Opaque token representing a connected slot (currently just a slot index).
///        Reserved for future use (e.g., disconnect by token).
typedef struct SignalToken {
    int slotId;
} SignalToken;

/// @brief Initialize a Signal to its empty state (all slots inactive).
/// @param sig The Signal to initialize (NULL is safe).
void signalInit(Signal* sig);

/// @brief Connect a callback to the signal.
/// @param sig      The Signal instance.
/// @param callback The callback function to invoke when the signal is emitted.
/// @param userData User-defined data passed to the callback.
/// @return Slot index (0-based) on success, or -1 if all slots are occupied.
int  signalConnect(Signal* sig, EngineCallback callback, void* userData);

/// @brief Disconnect a previously connected callback by its slot index.
/// @param sig    The Signal instance.
/// @param slotId The slot index returned by signalConnect.
void signalDisconnect(Signal* sig, int slotId);

/// @brief Emit the signal, calling all active connected callbacks.
/// @param sig    The Signal instance.
/// @param engine The Engine instance to pass to each callback.
void signalEmit(Signal* sig, Engine* engine);

/// @brief Remove all connected callbacks from the signal.
/// @param sig The Signal instance (NULL is safe).
void signalClear(Signal* sig);

#endif
