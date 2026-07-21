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

typedef struct EngineSnapshot {
    char text[ENGINE_SNAPSHOT_TEXT_MAX];
    size_t length;
    uint32_t cursorIndex;
    char expectedChar;
    bool incorrectFlags[ENGINE_SNAPSHOT_TEXT_MAX];

    SessionStats stats;
    EngineState state;
    EngineStopCause stopCause;
} EngineSnapshot;

EngineSnapshot engineGetSnapshot(Engine* engine);

#ifdef __cplusplus
}
#endif

#endif // SNAPSHOT_H