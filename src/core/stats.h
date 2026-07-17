#ifndef STATS_H
#define STATS_H

#include <stdint.h>

typedef struct Engine Engine; // Forward declaration of Engine struct

typedef struct {
    char timestamp[20];         ///< ISO 8601 format(YYYY-MM-DD HH:MM:SS) timestamp of the session
    uint32_t duration;          ///< Duration of the session in seconds
    uint32_t correctKeystrokes; ///< Number of correctly pressed keys
    uint32_t totalKeystrokes;   ///< Total number of keystrokes
    double accuracy;            ///< Accuracy of the session
    double wpm;                 ///< Words per minute (adjusted)
    double wpmRaw;              ///< Words per minute (raw)
} SessionStats;

SessionStats engineGetStats(Engine* engine);

#endif // STATS_H