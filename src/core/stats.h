#ifndef STATS_H
#define STATS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Engine Engine;

/// @brief Session statistics from engineGetStats.
typedef struct {
    char timestamp[20];           ///< ISO 8601 snapshot time (YYYY-MM-DD HH:MM:SS)
    int64_t durationMs;           ///< Elapsed session time in milliseconds
    uint32_t correctKeystrokes;   ///< Correctly pressed keys
    uint32_t incorrectKeystrokes; ///< Incorrectly pressed keys
    uint32_t totalKeystrokes;     ///< Total keystrokes (correct + incorrect)
    double accuracy;              ///< Accuracy percentage (correct / total * 100)
    double wpm;                   ///< WPM adjusted by accuracy (wpmRaw * accuracy / 100)
    double wpmRaw;                ///< Raw WPM (currentIndex / 5 / minutes)
} SessionStats;

/// @brief Get current session statistics.
/// @param engine Engine instance.
/// @return SessionStats with current statistics, zeroed if engine is NULL.
SessionStats engineGetStats(Engine* engine);

#ifdef __cplusplus
}
#endif

#endif // STATS_H