#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include "stats.h"
#include "state.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Engine Engine;

#define ENGINE_SNAPSHOT_TEXT_MAX 4096

/// @brief Complete session snapshot for UI rendering.
typedef struct EngineSnapshot {
    char text[ENGINE_SNAPSHOT_TEXT_MAX];      ///< Session text
    size_t length;                            ///< Text length
    uint32_t cursorIndex;                     ///< Current cursor position
    char expectedChar;                        ///< Character at cursor (NUL if done)
    bool incorrectFlags[ENGINE_SNAPSHOT_TEXT_MAX]; ///< Per-position error flags

    SessionStats stats;                       ///< Session statistics
    EngineState state;                        ///< Current engine state
    EngineStopCause stopCause;                ///< Stop reason
} EngineSnapshot;

/// @brief Get an atomic snapshot of the current session state.
/// @param engine Engine instance.
/// @return EngineSnapshot with current state. Zeroed if engine is NULL.
EngineSnapshot engineGetSnapshot(Engine* engine);

#ifdef __cplusplus
}
#endif

#endif // SNAPSHOT_H