#ifndef STATS_H
#define STATS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Engine Engine;

/// @brief Session statistics populated by engineGetStats.
typedef struct {
    char timestamp[20];           ///< ISO 8601 timestamp (YYYY-MM-DD HH:MM:SS) of the stats snapshot
    int64_t durationMs;           ///< Elapsed session time in milliseconds
    uint32_t correctKeystrokes;   ///< Number of correctly pressed keys
    uint32_t incorrectKeystrokes; ///< Number of incorrectly pressed keys
    uint32_t totalKeystrokes;     ///< Total number of keystrokes (including incorrect and correct)
    double accuracy;              ///< Accuracy percentage (correct / total * 100)
    double wpm;                   ///< Words per minute adjusted by accuracy (wpmRaw * accuracy / 100)
    double wpmRaw;                ///< Raw words per minute (currentIndex / 5 / minutes)
} SessionStats;

/// @brief Get the current session statistics.
/// @param engine The Engine instance.
/// @return A SessionStats struct with the current statistics.
///         Returns a zeroed struct if engine is NULL.
SessionStats engineGetStats(Engine* engine);

#ifdef __cplusplus
}
#endif

#endif // STATS_H
