#include "engine.h"
#include "callback.h"
#include "error.h"
#include "event.h"
#include "state.h"
#include "stats.h"
#include "logger.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
typedef LARGE_INTEGER ctypr_time_t;
#else
#include <time.h>
typedef struct timespec ctypr_time_t;
#endif

typedef struct Session {
    char text[4096]; // Assuming a maximum length for the text
    size_t length;
    uint32_t currentIndex;
    uint16_t incorrectKeystrokesIndices[4096]; // Array to track incorrect keystrokes

    ctypr_time_t segmentStartTime;
    ctypr_time_t segmentEndTime;
    int64_t accumulatedTimeMs; // Accumulated time in milliseconds
    bool isTimingStarted;      // Flag to indicate if timing has started
} Session;

// Cross-platform high-resolution time functions

static void getCurrentTime(ctypr_time_t* t) {
#ifdef _WIN32
    QueryPerformanceCounter(t);
#else
    clock_gettime(CLOCK_MONOTONIC, t);
#endif
}

static int64_t timeDiffMs(ctypr_time_t* end, ctypr_time_t* start) {
#ifdef _WIN32
    static LARGE_INTEGER freq = {0};
    if (freq.QuadPart == 0) {
        QueryPerformanceFrequency(&freq);
    }
    return (int64_t)((end->QuadPart - start->QuadPart) * 1000LL / freq.QuadPart);
#else
    int64_t secDiff = end->tv_sec - start->tv_sec;
    int64_t nsecDiff = end->tv_nsec - start->tv_nsec;
    return secDiff * 1000 + nsecDiff / 1000000;
#endif
}

// Helper functions

void clearArray(uint16_t* array, size_t size) {
    if (array) {
        for (size_t i = 0; i < size; ++i) {
            array[i] = 0;
        }
    }
}

void updateTime(Session* session) {
    if (!session) return;
    if (!session->isTimingStarted) return;
    getCurrentTime(&session->segmentEndTime);
    session->accumulatedTimeMs += timeDiffMs(&session->segmentEndTime, &session->segmentStartTime);
    if (session->accumulatedTimeMs < 0) { session->accumulatedTimeMs = 0; }
}

// engine.h

typedef struct Engine {
    EngineMode mode;
    uint16_t timeout;
    
    EngineState state;
    EngineError lastError;
    EngineStopCause stopCause;

    EngineCallback callbacks[MAX_CALLBACKS];
    void* callbackData[MAX_CALLBACKS];
    EngineEvent events[MAX_CALLBACKS];
    uint8_t callbackCount;

    Session* session;
    SessionStats stats;
    Logger* logger;
} Engine;

static void completeSession(Engine* self) {
    self->state = ENGINE_IDLE;
    self->stopCause = ENGINE_STOP_CAUSE_FINISHED;
    if (self->logger) loggerLog(self->logger, LOG_LEVEL_INFO, "Session completed");
    engineExecuteCallbacks(self, &(EngineEvent){ENGINE_EVENT_FINISHED});
    engineExecuteCallbacks(self, &(EngineEvent){ENGINE_EVENT_STOPPED});
}

/// @brief Check if the session has reached the end of the text.
///        If so, complete it and return true.
static bool tryCompleteSession(Engine* self) {
    if (self->session->currentIndex >= self->session->length) {
        completeSession(self);
        return true;
    }
    return false;
}

static bool checkTimeout(Engine* self) {
    if (!self || self->timeout == 0) return false;
    updateTime(self->session);
    if (self->session->accumulatedTimeMs >= (int64_t)self->timeout * 1000) {
        self->state = ENGINE_IDLE;
        self->stopCause = ENGINE_STOP_CAUSE_TIMEOUT;
        if (self->logger) loggerLog(self->logger, LOG_LEVEL_WARNING, "Session timed out");
        engineExecuteCallbacks(self, &(EngineEvent){ENGINE_EVENT_TIMEOUT});
        engineExecuteCallbacks(self, &(EngineEvent){ENGINE_EVENT_STOPPED});
        return true;
    }
    return false;
}

Engine* engineCreate(EngineMode mode, uint16_t timeout) {
    Engine* engine = malloc(sizeof(Engine));
    if (!engine) return NULL;
    engine->mode = mode;
    engine->timeout = timeout;
    engine->state = ENGINE_IDLE;
    engine->lastError = ENGINE_ERROR_NONE;
    engine->stopCause = ENGINE_STOP_CAUSE_NONE;
    engine->callbackCount = 0;
    engine->logger = NULL;
    engine->session = malloc(sizeof(Session));
    if (!engine->session) {
        free(engine);
        return NULL;
    }
    memset(&engine->stats, 0, sizeof(SessionStats));
    engine->session->isTimingStarted = false;
    clearArray(engine->session->incorrectKeystrokesIndices, 4096);
    return engine;
}

void engineDestroy(Engine *self) {
    if (self) {
        if (self->logger) loggerLog(self->logger, LOG_LEVEL_DEBUG, "Engine destroyed");
        if (self->session) { free(self->session); }
        free(self);
    }
}

void engineSetLogger(Engine* self, Logger* logger) {
    if (self) self->logger = logger;
}

void engineSetMode(Engine *self, EngineMode mode) {
    if (self) { self->mode = mode; }
}

EngineMode engineGetMode(Engine *self) {
    if (self) { return self->mode; }
    return UnknownMode; // Default return value
}

void engineSetTimeout(Engine *self, uint16_t timeout) {
    if (self) { self->timeout = timeout; }
}

uint16_t engineGetTimeout(Engine *self) {
    if (self) { return self->timeout; }
    return 0; // Default return value
}

void engineStart(Engine *self) {
    if (self) {
        if (self->state == ENGINE_RUNNING) {
            self->lastError = ENGINE_ERROR_ALREADY_RUNNING;
            if (self->logger) loggerLog(self->logger, LOG_LEVEL_WARNING, "engineStart: already running");
            return;
        }
        self->state = ENGINE_RUNNING;
        self->lastError = ENGINE_ERROR_NONE;
        self->stopCause = ENGINE_STOP_CAUSE_NONE;
        memset(&self->stats, 0, sizeof(SessionStats));
        snprintf(self->session->text, sizeof(self->session->text), "%s", "The quick brown fox jumps over the lazy dog.");
        self->session->length = strlen(self->session->text);
        self->session->currentIndex = 0;
        getCurrentTime(&self->session->segmentStartTime);
        self->session->isTimingStarted = true;
        self->session->accumulatedTimeMs = 0;
        clearArray(self->session->incorrectKeystrokesIndices, 4096);
        if (self->logger) loggerLog(self->logger, LOG_LEVEL_INFO, "Session started");
        engineExecuteCallbacks(self, &(EngineEvent){ENGINE_EVENT_STARTED});
    }
}

void engineStop(Engine *self) {
    if (self) {
        if (self->state != ENGINE_RUNNING) {
            self->lastError = ENGINE_ERROR_NOT_RUNNING;
            if (self->logger) loggerLog(self->logger, LOG_LEVEL_WARNING, "engineStop: not running");
            return;
        }
        self->state = ENGINE_IDLE;
        self->lastError = ENGINE_ERROR_NONE;
        self->stopCause = ENGINE_STOP_CAUSE_USER;
        updateTime(self->session);
        if (self->logger) loggerLog(self->logger, LOG_LEVEL_INFO, "Session stopped by user");
        engineExecuteCallbacks(self, &(EngineEvent){ENGINE_EVENT_STOPPED});
    }
}

void enginePause(Engine *self) {
    if (self) {
        if (self->state != ENGINE_RUNNING) {
            self->lastError = ENGINE_ERROR_NOT_RUNNING;
            if (self->logger) loggerLog(self->logger, LOG_LEVEL_WARNING, "enginePause: not running");
            return;
        }
        self->state = ENGINE_PAUSED;
        self->lastError = ENGINE_ERROR_NONE;
        updateTime(self->session);
        if (self->logger) loggerLog(self->logger, LOG_LEVEL_INFO, "Session paused");
        engineExecuteCallbacks(self, &(EngineEvent){ENGINE_EVENT_PAUSED});
    }
}

void engineResume(Engine *self) {
    if (self) {
        if (self->state != ENGINE_PAUSED) {
            self->lastError = ENGINE_ERROR_NOT_RUNNING;
            if (self->logger) loggerLog(self->logger, LOG_LEVEL_WARNING, "engineResume: not paused");
            return;
        }
        self->state = ENGINE_RUNNING;
        self->lastError = ENGINE_ERROR_NONE;
        getCurrentTime(&self->session->segmentStartTime);
        self->session->isTimingStarted = true;
        if (self->logger) loggerLog(self->logger, LOG_LEVEL_INFO, "Session resumed");
        engineExecuteCallbacks(self, &(EngineEvent){ENGINE_EVENT_RESUMED});
    }
}

bool engineIsRunning(Engine* self)   { return self && self->state == ENGINE_RUNNING; }
bool engineIsPaused(Engine* self)    { return self && self->state == ENGINE_PAUSED; }
bool engineIsIdle(Engine* self)      { return self && self->state == ENGINE_IDLE && self->stopCause == ENGINE_STOP_CAUSE_NONE; }
bool engineIsError(Engine* self)     { return self && self->state == ENGINE_ERROR; }
bool engineIsCompleted(Engine* self) { return self && self->state == ENGINE_IDLE && self->stopCause == ENGINE_STOP_CAUSE_FINISHED; }
bool engineIsTimedOut(Engine* self)  { return self && self->state == ENGINE_IDLE && self->stopCause == ENGINE_STOP_CAUSE_TIMEOUT; }
bool engineIsStopped(Engine* self)   { return self && self->state == ENGINE_IDLE && self->stopCause == ENGINE_STOP_CAUSE_USER; }

void engineReset(Engine *self) {
    if (self) {
        self->state = ENGINE_IDLE;
        self->lastError = ENGINE_ERROR_NONE;
        self->stopCause = ENGINE_STOP_CAUSE_NONE;
        memset(&self->stats, 0, sizeof(SessionStats));
        self->session->text[0] = '\0';
        self->session->length = 0;
        self->session->currentIndex = 0;
        self->session->isTimingStarted = false;
        self->session->accumulatedTimeMs = 0;
        clearArray(self->session->incorrectKeystrokesIndices, 4096);
        if (self->logger) loggerLog(self->logger, LOG_LEVEL_INFO, "Session reset");
        engineExecuteCallbacks(self, &(EngineEvent){ENGINE_EVENT_NONE});
    }
}

void engineKeyPress_Strict(Engine *self, char key) {
    if (!self || self->state != ENGINE_RUNNING) return;
    if (checkTimeout(self)) return;
    if (tryCompleteSession(self)) return;

    char expectedChar = self->session->text[self->session->currentIndex];
    self->stats.totalKeystrokes++;
    if (key == expectedChar) {
        self->session->currentIndex++;
        self->stats.correctKeystrokes++;
        if (self->logger) loggerLog(self->logger, LOG_LEVEL_DEBUG, "Strict: correct key");
        engineExecuteCallbacks(self, &(EngineEvent){ENGINE_EVENT_CORRECT_KEYSTROKE});
        tryCompleteSession(self);
    } else {
        self->session->incorrectKeystrokesIndices[self->session->currentIndex] = 1;
        if (self->logger) loggerLog(self->logger, LOG_LEVEL_DEBUG, "Strict: incorrect key");
        engineExecuteCallbacks(self, &(EngineEvent){ENGINE_EVENT_INCORRECT_KEYSTROKE});
    }
}

void engineKeyPress_Flow(Engine *self, char key) {
    if (!self || self->state != ENGINE_RUNNING) return;
    if (checkTimeout(self)) return;
    if (tryCompleteSession(self)) return;

    char expectedChar = self->session->text[self->session->currentIndex];
    self->stats.totalKeystrokes++;
    if (key == expectedChar) {
        self->stats.correctKeystrokes++;
        if (self->logger) loggerLog(self->logger, LOG_LEVEL_DEBUG, "Flow: correct key");
        engineExecuteCallbacks(self, &(EngineEvent){ENGINE_EVENT_CORRECT_KEYSTROKE});
    } else {
        self->session->incorrectKeystrokesIndices[self->session->currentIndex] = 1;
        if (self->logger) loggerLog(self->logger, LOG_LEVEL_DEBUG, "Flow: incorrect key");
        engineExecuteCallbacks(self, &(EngineEvent){ENGINE_EVENT_INCORRECT_KEYSTROKE});
    }
    self->session->currentIndex++;
    tryCompleteSession(self);
}

void engineKeyPress(Engine *self, char key) {
    if (!self) return;
    if (self->mode == StrictMode) {
        engineKeyPress_Strict(self, key);
    } else if (self->mode == FlowMode) {
        engineKeyPress_Flow(self, key);
    }
}

void engineBackspacePress_Strict(Engine *self) {
    (void)self; // Backspace is disabled in Strict Mode
}

void engineBackspacePress_Flow(Engine *self) {
    if (!self || self->state != ENGINE_RUNNING) return;
    if (self->session->currentIndex <= 0) return;
    self->session->currentIndex--;
    uint16_t *was_incorrect = &self->session->incorrectKeystrokesIndices[self->session->currentIndex];
    if (*was_incorrect == 1) {
        *was_incorrect = 0;
        if (self->logger) loggerLog(self->logger, LOG_LEVEL_DEBUG, "Flow: backspace over incorrect key");
    } else {
        self->stats.correctKeystrokes--;
        if (self->logger) loggerLog(self->logger, LOG_LEVEL_DEBUG, "Flow: backspace over correct key");
    }
    engineExecuteCallbacks(self, &(EngineEvent){ENGINE_EVENT_BACKSPACE});
}

void engineBackspacePress(Engine *self) {
    if (!self) return;
    if (self->mode == StrictMode) {
        engineBackspacePress_Strict(self);
    } else if (self->mode == FlowMode) {
        engineBackspacePress_Flow(self);
    }
}

// callback.h

bool engineRegisterCallback(Engine *engine, EngineEvent* event, EngineCallback callback, void *userData) {
    if (!engine || !callback) return false;
    if (engine->callbackCount >= MAX_CALLBACKS) return false;
    if (event) {
        engine->events[engine->callbackCount] = *event;
    } else {
        engine->events[engine->callbackCount] = ENGINE_EVENT_NONE; // Default event if none provided
    }
    engine->callbacks[engine->callbackCount] = callback;
    engine->callbackData[engine->callbackCount] = userData;
    engine->callbackCount++;
    return true;
}

void engineExecuteCallbacks(Engine *engine, EngineEvent* event) {
    if (!engine) return;
    if (!event) return; // No event provided
    for (uint8_t i = 0; i < engine->callbackCount; ++i) {
        EngineCallback callback = engine->callbacks[i];
        EngineEvent registeredEvent = engine->events[i];
        if (registeredEvent == *event && callback) {
            callback(engine, engine->callbackData[i]);
        }
    }
}

// error.h

void engineErrorToString(EngineError error, char* buffer, size_t bufferSize) {
    const char* errorString = "Unknown Error";
    switch (error) {
        case ENGINE_ERROR_NONE: errorString = "No Error"; break;
        case ENGINE_ERROR_INVALID_MODE: errorString = "Invalid Mode"; break;
        case ENGINE_ERROR_INVALID_TIMEOUT: errorString = "Invalid Timeout"; break;
        case ENGINE_ERROR_ALREADY_RUNNING: errorString = "Already Running"; break;
        case ENGINE_ERROR_NOT_RUNNING: errorString = "Not Running"; break;
        case ENGINE_ERROR_UNKNOWN: errorString = "Unknown Error"; break;
    }
    snprintf(buffer, bufferSize, "%s", errorString);
}

// event.h

void engineEventToString(EngineEvent event, char* buffer, size_t bufferSize) {
    const char* eventString = "Unknown Event";
    switch (event) {
        case ENGINE_EVENT_NONE: eventString = "No Event"; break;
        case ENGINE_EVENT_STARTED: eventString = "Started"; break;
        case ENGINE_EVENT_STOPPED: eventString = "Stopped"; break;
        case ENGINE_EVENT_PAUSED: eventString = "Paused"; break;
        case ENGINE_EVENT_RESUMED: eventString = "Resumed"; break;
        case ENGINE_EVENT_TIMEOUT: eventString = "Timeout"; break;
        case ENGINE_EVENT_FINISHED: eventString = "Finished"; break;
        case ENGINE_EVENT_CORRECT_KEYSTROKE: eventString = "Correct Keystroke"; break;
        case ENGINE_EVENT_INCORRECT_KEYSTROKE: eventString = "Incorrect Keystroke"; break;
        case ENGINE_EVENT_BACKSPACE: eventString = "Backspace Pressed"; break;
        case ENGINE_EVENT_SEGMENT_COMPLETED: eventString = "Segment Completed"; break;
        case ENGINE_EVENT_ERROR: eventString = "Error Occurred"; break;
    }
    snprintf(buffer, bufferSize, "%s", eventString);
}

void onEngineEvent(Engine *engine, EngineEvent event, EngineCallback callback, void *userData) {
    if (!engine || !callback) return;
    engineRegisterCallback(engine, &event, callback, userData);
}

// state.h

EngineStateInfo engineGetStateInfo(Engine* engine) {
    EngineStateInfo info;
    if (!engine) {
        info.state = ENGINE_ERROR;
        info.stopCause = ENGINE_STOP_CAUSE_ERROR;
        return info;
    }
    info.state = engine->state;
    info.stopCause = engine->stopCause;
    return info;
}

// stats.h

SessionStats engineGetStats(Engine* engine) {
    SessionStats stats;
    memset(&stats, 0, sizeof(SessionStats));
    if (!engine || !engine->session) {
        return stats;
    }
    // Check timeout on every stats poll (UI updates trigger this)
    checkTimeout(engine);
    // Copy raw counters
    stats.totalKeystrokes = engine->stats.totalKeystrokes;
    stats.correctKeystrokes = engine->stats.correctKeystrokes;
    // Calculate duration in milliseconds
    stats.durationMs = engine->session->accumulatedTimeMs;
    // Calculate accuracy
    if (engine->stats.totalKeystrokes > 0) {
        stats.accuracy = (double)engine->stats.correctKeystrokes / engine->stats.totalKeystrokes * 100.0;
    } else {
        stats.accuracy = 100.0;
    }
    // Calculate WPM (Words Per Minute)
    double minutes = (double)stats.durationMs / 60000.0;
    if (minutes > 0) {
        stats.wpmRaw = (double)engine->session->currentIndex / 5.0 / minutes; // Assuming 5 characters per word
        stats.wpm = stats.wpmRaw * (stats.accuracy / 100.0); // Adjusted WPM based on accuracy
    } else {
        stats.wpmRaw = 0.0;
        stats.wpm = 0.0;
    }
    // Set timestamp
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(stats.timestamp, sizeof(stats.timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    return stats;
}
