#ifndef ENGINE_INTERNAL_H
#define ENGINE_INTERNAL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Engine Engine;

#ifdef _WIN32
#include <windows.h>
typedef LARGE_INTEGER ctypr_time_t;
#else
#include <time.h>
typedef struct timespec ctypr_time_t;
#endif

typedef struct Session {
    char text[4096];
    size_t length;
    uint32_t currentIndex;
    uint8_t incorrectKeystrokesBitmap[4096];
    ctypr_time_t segmentStartTime;
    ctypr_time_t segmentEndTime;
    int64_t accumulatedTimeMs;
    bool isTimingStarted;
    char cachedTimestamp[20];
} Session;

Session* engineGetSession(Engine* self);
void updateTime(Session* session);

#ifdef __cplusplus
}
#endif

#endif
