#include "engine.h"
#include "callback.h"
#include "error.h"
#include "event.h"
#include "state.h"
#include "stats.h"
#include "snapshot.h"
#include "logger.h"
#include "signal_internal.h"
#include "content.h"
#include "repository.h"

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
    if (nsecDiff < 0) {
        secDiff--;
        nsecDiff += 1000000000;
    }
    return secDiff * 1000 + nsecDiff / 1000000;
#endif
}

void updateTime(Session* session) {
    if (!session) return;
    if (!session->isTimingStarted) return;
    getCurrentTime(&session->segmentEndTime);
    session->accumulatedTimeMs += timeDiffMs(&session->segmentEndTime, &session->segmentStartTime);
    if (session->accumulatedTimeMs < 0) { session->accumulatedTimeMs = 0; }
    session->segmentStartTime = session->segmentEndTime;
}

// engine.h

typedef struct Engine {
    EngineMode mode;
    uint16_t timeout;
    
    EngineState state;
    EngineError lastError;
    EngineStopCause stopCause;

    Signal onStarted, onStopped, onPaused, onResumed;
    Signal onTimeout, onFinished, onCorrectKeystroke, onIncorrectKeystroke;
    Signal onBackspace, onSegmentCompleted, onError;

    Session* session;
    SessionStats stats;
    Logger* logger;
    const char* name;
    ContentProvider* contentProvider;
    Repository* autoSaveRepo;
    bool autoSaveEnabled;
} Engine;

static const char* modeToString(EngineMode mode) {
    switch (mode) {
        case StrictMode: return "strict";
        case FlowMode:   return "flow";
        default:         return "unknown";
    }
}

static void cacheTimestamp(char* buf, size_t size) {
    time_t now = time(NULL);
#ifdef _WIN32
    struct tm tm_info;
    localtime_s(&tm_info, &now);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", &tm_info);
#else
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", &tm_info);
#endif
}

static void computeSessionStats(SessionStats* stats, uint32_t correctChars, uint32_t totalChars, uint32_t currentIndex, int64_t durationMs) {
    if (totalChars > 0) {
        stats->accuracy = (double)correctChars / totalChars * 100.0;
    } else {
        stats->accuracy = 100.0;
    }
    double minutes = (double)durationMs / 60000.0;
    if (minutes > 0) {
        stats->wpmRaw = (double)currentIndex / 5.0 / minutes;
        stats->wpm = stats->wpmRaw * (stats->accuracy / 100.0);
    } else {
        stats->wpmRaw = 0.0;
        stats->wpm = 0.0;
    }
}

static void autoSaveSession(Engine* self) {
    if (!self->autoSaveEnabled || !self->autoSaveRepo) return;
    SessionData data;
    memset(&data, 0, sizeof(data));
    snprintf(data.mode, sizeof(data.mode), "%s", modeToString(self->mode));
    data.totalChars   = self->stats.totalKeystrokes;
    data.correctChars = self->stats.correctKeystrokes;
    data.durationMs   = self->session->accumulatedTimeMs;
    {
        SessionStats tmp;
        computeSessionStats(&tmp, self->stats.correctKeystrokes, self->stats.totalKeystrokes, self->session->currentIndex, data.durationMs);
        data.accuracy = tmp.accuracy;
        data.wpm = tmp.wpm;
        data.wpmRaw = tmp.wpmRaw;
    }
    memcpy(data.timestamp, self->session->cachedTimestamp, sizeof(data.timestamp));
    int64_t id = repositorySaveSession(self->autoSaveRepo, &data);
    if (self->logger && id >= 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Session auto-saved with ID: %lld", (long long)id);
        loggerLog(self->logger, LOG_LEVEL_INFO, buf);
    }
}

static void completeSession(Engine* self) {
    self->state = ENGINE_IDLE;
    self->stopCause = ENGINE_STOP_CAUSE_FINISHED;
    if (self->logger) loggerLog(self->logger, LOG_LEVEL_INFO, "Session completed");
    cacheTimestamp(self->session->cachedTimestamp, sizeof(self->session->cachedTimestamp));
    autoSaveSession(self);
    signalEmit(&self->onSegmentCompleted, self);
    signalEmit(&self->onFinished, self);
    signalEmit(&self->onStopped, self);
}

static bool tryCompleteSession(Engine* self) {
    if (self->session->currentIndex >= self->session->length) {
        completeSession(self);
        return true;
    }
    return false;
}

static bool checkTimeout(Engine* self) {
    if (!self || self->timeout == 0 || !self->session) return false;
    if (self->state == ENGINE_RUNNING) {
        updateTime(self->session);
    }
    if (self->session->accumulatedTimeMs >= (int64_t)self->timeout * 1000LL) {
        self->state = ENGINE_IDLE;
        self->stopCause = ENGINE_STOP_CAUSE_TIMEOUT;
        cacheTimestamp(self->session->cachedTimestamp, sizeof(self->session->cachedTimestamp));
        if (self->logger) loggerLog(self->logger, LOG_LEVEL_WARNING, "Session timed out");
        autoSaveSession(self);
        signalEmit(&self->onTimeout, self);
        signalEmit(&self->onStopped, self);
        return true;
    }
    return false;
}

Engine* engineCreate(const EngineConfig* config) {
    if (!config || config->mode == UnknownMode || !config->contentProvider) {
        return NULL;
    }
    Engine* engine = calloc(1, sizeof(Engine));
    if (!engine) {
        fprintf(stderr, "[ERROR] Failed to allocate Engine\n");
        return NULL;
    }
    engine->mode = config->mode;
    engine->timeout = config->timeout;
    engine->state = ENGINE_IDLE;
    engine->lastError = ENGINE_ERROR_NONE;
    engine->stopCause = ENGINE_STOP_CAUSE_NONE;
    engine->logger = NULL;
    engine->name = "Engine";
    engine->contentProvider = config->contentProvider;
    engine->autoSaveRepo = config->autoSaveRepo;
    engine->autoSaveEnabled = config->autoSaveEnabled;
    engine->session = calloc(1, sizeof(Session));
    if (!engine->session) {
        fprintf(stderr, "[ERROR] Failed to allocate Engine session\n");
        free(engine);
        return NULL;
    }
    memset(&engine->stats, 0, sizeof(SessionStats));
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
    if (self) {
        self->logger = logger;
        if (self->contentProvider) contentProviderSetLogger(self->contentProvider, logger);
        if (self->autoSaveRepo) repositorySetLogger(self->autoSaveRepo, logger);
    }
}

void engineSetContentProvider(Engine* self, ContentProvider* provider) {
    if (self) {
        self->contentProvider = provider;
        if (provider && self->logger) contentProviderSetLogger(provider, self->logger);
    }
}

void engineSetAutoSave(Engine* self, Repository* repo, bool enabled) {
    if (self) {
        self->autoSaveRepo = repo;
        self->autoSaveEnabled = enabled;
    }
}

void engineSetMode(Engine *self, EngineMode mode) {
    if (self) { self->mode = mode; }
}

EngineMode engineGetMode(Engine *self) {
    if (self) { return self->mode; }
    return UnknownMode;
}

void engineSetTimeout(Engine *self, uint16_t timeout) {
    if (self) { self->timeout = timeout; }
}

uint16_t engineGetTimeout(Engine *self) {
    if (self) { return self->timeout; }
    return 0;
}

void engineStart(Engine *self) {
    if (self) {
        if (self->state == ENGINE_RUNNING) {
            self->lastError = ENGINE_ERROR_ALREADY_RUNNING;
            if (self->logger) loggerLog(self->logger, LOG_LEVEL_WARNING, "engineStart: already running");
            return;
        }
        if (!self->contentProvider) {
            self->lastError = ENGINE_ERROR_CONTENT;
            self->state = ENGINE_ERROR;
            if (self->logger) loggerLog(self->logger, LOG_LEVEL_ERROR, "engineStart: no content provider");
            return;
        }
        ContentChunk chunk = contentProviderGetNext(self->contentProvider);
        if (chunk.length == 0) {
            self->lastError = ENGINE_ERROR_CONTENT;
            self->state = ENGINE_ERROR;
            if (self->logger) loggerLog(self->logger, LOG_LEVEL_ERROR, "engineStart: content provider returned empty text");
            return;
        }
        self->state = ENGINE_RUNNING;
        self->lastError = ENGINE_ERROR_NONE;
        self->stopCause = ENGINE_STOP_CAUSE_NONE;
        memset(&self->stats, 0, sizeof(SessionStats));
        snprintf(self->session->text, sizeof(self->session->text), "%s", chunk.text);
        self->session->length = strlen(self->session->text);
        self->session->currentIndex = 0;
        getCurrentTime(&self->session->segmentStartTime);
        self->session->isTimingStarted = true;
        self->session->accumulatedTimeMs = 0;
        memset(self->session->incorrectKeystrokesBitmap, 0, sizeof(self->session->incorrectKeystrokesBitmap));
        cacheTimestamp(self->session->cachedTimestamp, sizeof(self->session->cachedTimestamp));
        if (self->logger) loggerLog(self->logger, LOG_LEVEL_INFO, "Session started");
        signalEmit(&self->onStarted, self);
    }
}

void engineStop(Engine *self) {
    if (self) {
        if (self->state != ENGINE_RUNNING) {
            if (self->stopCause == ENGINE_STOP_CAUSE_FINISHED ||
                self->stopCause == ENGINE_STOP_CAUSE_TIMEOUT) {
                return;
            }
            self->lastError = ENGINE_ERROR_NOT_RUNNING;
            if (self->logger) loggerLog(self->logger, LOG_LEVEL_WARNING, "engineStop: not running");
            return;
        }
        self->state = ENGINE_IDLE;
        self->lastError = ENGINE_ERROR_NONE;
        self->stopCause = ENGINE_STOP_CAUSE_USER;
        updateTime(self->session);
        self->session->isTimingStarted = false;
        cacheTimestamp(self->session->cachedTimestamp, sizeof(self->session->cachedTimestamp));
        if (self->logger) loggerLog(self->logger, LOG_LEVEL_INFO, "Session stopped by user");
        autoSaveSession(self);
        signalEmit(&self->onStopped, self);
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
        signalEmit(&self->onPaused, self);
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
        signalEmit(&self->onResumed, self);
    }
}

bool engineIsRunning(Engine* self)   { return self && self->state == ENGINE_RUNNING; }
bool engineIsPaused(Engine* self)    { return self && self->state == ENGINE_PAUSED; }
bool engineIsIdle(Engine* self)      { return self && self->state == ENGINE_IDLE && self->stopCause == ENGINE_STOP_CAUSE_NONE; }
bool engineIsError(Engine* self)     { return self && self->state == ENGINE_ERROR; }
bool engineIsCompleted(Engine* self) { return self && self->state == ENGINE_IDLE && self->stopCause == ENGINE_STOP_CAUSE_FINISHED; }
bool engineIsTimedOut(Engine* self)  { return self && self->state == ENGINE_IDLE && self->stopCause == ENGINE_STOP_CAUSE_TIMEOUT; }
bool engineIsStopped(Engine* self)   { return self && self->state == ENGINE_IDLE && self->stopCause == ENGINE_STOP_CAUSE_USER; }
bool engineWasStopped(Engine* self)  { return self && self->stopCause == ENGINE_STOP_CAUSE_USER; }

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
        memset(self->session->incorrectKeystrokesBitmap, 0, sizeof(self->session->incorrectKeystrokesBitmap));
        if (self->logger) loggerLog(self->logger, LOG_LEVEL_INFO, "Session reset");
    }
}

static void engineKeyPressInternal(Engine *self, char key, bool advanceAlways) {
    if (!self || self->state != ENGINE_RUNNING) return;
    if (checkTimeout(self)) return;
    if (tryCompleteSession(self)) return;

    char expectedChar = self->session->text[self->session->currentIndex];
    self->stats.totalKeystrokes++;
    if (key == expectedChar) {
        self->stats.correctKeystrokes++;
        if (self->logger) loggerLog(self->logger, LOG_LEVEL_DEBUG, "Correct key");
        signalEmit(&self->onCorrectKeystroke, self);
    } else {
        self->session->incorrectKeystrokesBitmap[self->session->currentIndex] = 1;
        self->stats.incorrectKeystrokes++;
        if (self->logger) loggerLog(self->logger, LOG_LEVEL_DEBUG, "Incorrect key");
        signalEmit(&self->onIncorrectKeystroke, self);
    }
    if (advanceAlways || key == expectedChar) {
        self->session->currentIndex++;
        tryCompleteSession(self);
    }
}

void engineKeyPress(Engine *self, char key) {
    if (!self) return;
    if (self->mode == StrictMode) {
        engineKeyPressInternal(self, key, false);
    } else if (self->mode == FlowMode) {
        engineKeyPressInternal(self, key, true);
    }
}

void engineBackspacePress_Strict(Engine *self) {
    (void)self;
}

void engineBackspacePress_Flow(Engine *self) {
    if (!self || self->state != ENGINE_RUNNING) return;
    if (checkTimeout(self)) return;
    if (self->session->currentIndex <= 0) return;
    self->session->currentIndex--;
    uint8_t *was_incorrect = &self->session->incorrectKeystrokesBitmap[self->session->currentIndex];
    if (*was_incorrect == 1) {
        *was_incorrect = 0;
        if (self->logger) loggerLog(self->logger, LOG_LEVEL_DEBUG, "Flow: backspace over incorrect key");
    } else {
        self->stats.correctKeystrokes--;
        if (self->logger) loggerLog(self->logger, LOG_LEVEL_DEBUG, "Flow: backspace over correct key");
    }
    signalEmit(&self->onBackspace, self);
}

void engineBackspacePress(Engine *self) {
    if (!self) return;
    if (self->mode == StrictMode) {
        engineBackspacePress_Strict(self);
    } else if (self->mode == FlowMode) {
        engineBackspacePress_Flow(self);
    }
}

// callback.h — per-event registration

int engineOnStarted(Engine* engine, EngineCallback callback, void* userData) {
    if (!engine) return -1;
    return signalConnect(&engine->onStarted, callback, userData);
}

int engineOnStopped(Engine* engine, EngineCallback callback, void* userData) {
    if (!engine) return -1;
    return signalConnect(&engine->onStopped, callback, userData);
}

int engineOnPaused(Engine* engine, EngineCallback callback, void* userData) {
    if (!engine) return -1;
    return signalConnect(&engine->onPaused, callback, userData);
}

int engineOnResumed(Engine* engine, EngineCallback callback, void* userData) {
    if (!engine) return -1;
    return signalConnect(&engine->onResumed, callback, userData);
}

int engineOnTimeout(Engine* engine, EngineCallback callback, void* userData) {
    if (!engine) return -1;
    return signalConnect(&engine->onTimeout, callback, userData);
}

int engineOnFinished(Engine* engine, EngineCallback callback, void* userData) {
    if (!engine) return -1;
    return signalConnect(&engine->onFinished, callback, userData);
}

int engineOnCorrectKeystroke(Engine* engine, EngineCallback callback, void* userData) {
    if (!engine) return -1;
    return signalConnect(&engine->onCorrectKeystroke, callback, userData);
}

int engineOnIncorrectKeystroke(Engine* engine, EngineCallback callback, void* userData) {
    if (!engine) return -1;
    return signalConnect(&engine->onIncorrectKeystroke, callback, userData);
}

int engineOnBackspace(Engine* engine, EngineCallback callback, void* userData) {
    if (!engine) return -1;
    return signalConnect(&engine->onBackspace, callback, userData);
}

int engineOnSegmentCompleted(Engine* engine, EngineCallback callback, void* userData) {
    if (!engine) return -1;
    return signalConnect(&engine->onSegmentCompleted, callback, userData);
}

int engineOnError(Engine* engine, EngineCallback callback, void* userData) {
    if (!engine) return -1;
    return signalConnect(&engine->onError, callback, userData);
}

void engineDisconnect(Engine* engine, EngineEvent event, int slotId) {
    if (!engine) return;
    Signal* sig = NULL;
    switch (event) {
        case ENGINE_EVENT_STARTED:   sig = &engine->onStarted; break;
        case ENGINE_EVENT_STOPPED:   sig = &engine->onStopped; break;
        case ENGINE_EVENT_PAUSED:    sig = &engine->onPaused; break;
        case ENGINE_EVENT_RESUMED:   sig = &engine->onResumed; break;
        case ENGINE_EVENT_TIMEOUT:   sig = &engine->onTimeout; break;
        case ENGINE_EVENT_FINISHED:  sig = &engine->onFinished; break;
        case ENGINE_EVENT_CORRECT_KEYSTROKE:   sig = &engine->onCorrectKeystroke; break;
        case ENGINE_EVENT_INCORRECT_KEYSTROKE: sig = &engine->onIncorrectKeystroke; break;
        case ENGINE_EVENT_BACKSPACE: sig = &engine->onBackspace; break;
        case ENGINE_EVENT_SEGMENT_COMPLETED: sig = &engine->onSegmentCompleted; break;
        case ENGINE_EVENT_ERROR:     sig = &engine->onError; break;
        default: return;
    }
    signalDisconnect(sig, slotId);
}

void engineClearEvent(Engine* engine, EngineEvent event) {
    if (!engine) return;
    Signal* sig = NULL;
    switch (event) {
        case ENGINE_EVENT_STARTED:   sig = &engine->onStarted; break;
        case ENGINE_EVENT_STOPPED:   sig = &engine->onStopped; break;
        case ENGINE_EVENT_PAUSED:    sig = &engine->onPaused; break;
        case ENGINE_EVENT_RESUMED:   sig = &engine->onResumed; break;
        case ENGINE_EVENT_TIMEOUT:   sig = &engine->onTimeout; break;
        case ENGINE_EVENT_FINISHED:  sig = &engine->onFinished; break;
        case ENGINE_EVENT_CORRECT_KEYSTROKE:   sig = &engine->onCorrectKeystroke; break;
        case ENGINE_EVENT_INCORRECT_KEYSTROKE: sig = &engine->onIncorrectKeystroke; break;
        case ENGINE_EVENT_BACKSPACE: sig = &engine->onBackspace; break;
        case ENGINE_EVENT_SEGMENT_COMPLETED: sig = &engine->onSegmentCompleted; break;
        case ENGINE_EVENT_ERROR:     sig = &engine->onError; break;
        default: return;
    }
    signalClear(sig);
}

// error.h

void engineErrorToString(EngineError error, char* buffer, size_t bufferSize) {
    if (!buffer) return;
    const char* errorString = "Unknown Error";
    switch (error) {
        case ENGINE_ERROR_NONE: errorString = "No Error"; break;
        case ENGINE_ERROR_INVALID_MODE: errorString = "Invalid Mode"; break;
        case ENGINE_ERROR_INVALID_TIMEOUT: errorString = "Invalid Timeout"; break;
        case ENGINE_ERROR_ALREADY_RUNNING: errorString = "Already Running"; break;
        case ENGINE_ERROR_NOT_RUNNING: errorString = "Not Running"; break;
        case ENGINE_ERROR_CONFIG: errorString = "Config Error"; break;
        case ENGINE_ERROR_CONTENT: errorString = "Content Error"; break;
        case ENGINE_ERROR_STATE: errorString = "State Error"; break;
        case ENGINE_ERROR_PROVIDER: errorString = "Provider Error"; break;
        case ENGINE_ERROR_FILE: errorString = "File Error"; break;
        case ENGINE_ERROR_UNKNOWN: errorString = "Unknown Error"; break;
    }
    snprintf(buffer, bufferSize, "%s", errorString);
}

// event.h

void engineEventToString(EngineEvent event, char* buffer, size_t bufferSize) {
    if (!buffer) return;
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
    if (engine->state == ENGINE_RUNNING && engine->session->isTimingStarted) {
        updateTime(engine->session);
    }
    stats.totalKeystrokes = engine->stats.totalKeystrokes;
    stats.correctKeystrokes = engine->stats.correctKeystrokes;
    stats.incorrectKeystrokes = engine->stats.incorrectKeystrokes;
    stats.durationMs = engine->session->accumulatedTimeMs;
    computeSessionStats(&stats, engine->stats.correctKeystrokes, engine->stats.totalKeystrokes, engine->session->currentIndex, stats.durationMs);
    memcpy(stats.timestamp, engine->session->cachedTimestamp, sizeof(stats.timestamp));

    return stats;
}

// snapshot.h

EngineSnapshot engineGetSnapshot(Engine* engine) {
    EngineSnapshot snapshot;
    memset(&snapshot, 0, sizeof(EngineSnapshot));

    if (!engine || !engine->session) {
        snapshot.state = ENGINE_ERROR;
        snapshot.stopCause = ENGINE_STOP_CAUSE_ERROR;
        return snapshot;
    }

    if (engine->state == ENGINE_RUNNING && engine->session->isTimingStarted) {
        updateTime(engine->session);
    }

    memcpy(snapshot.text, engine->session->text, sizeof(engine->session->text));
    snapshot.length = engine->session->length;
    snapshot.cursorIndex = engine->session->currentIndex;

    if (engine->session->currentIndex < engine->session->length) {
        snapshot.expectedChar = engine->session->text[engine->session->currentIndex];
    } else {
        snapshot.expectedChar = '\0';
    }

    for (size_t i = 0; i < engine->session->length && i < ENGINE_SNAPSHOT_TEXT_MAX; i++) {
        snapshot.incorrectFlags[i] = engine->session->incorrectKeystrokesBitmap[i] != 0;
    }

    // Populate SessionStats inline
    snapshot.stats.totalKeystrokes = engine->stats.totalKeystrokes;
    snapshot.stats.correctKeystrokes = engine->stats.correctKeystrokes;
    snapshot.stats.incorrectKeystrokes = engine->stats.incorrectKeystrokes;
    snapshot.stats.durationMs = engine->session->accumulatedTimeMs;
    computeSessionStats(&snapshot.stats,
                        engine->stats.correctKeystrokes,
                        engine->stats.totalKeystrokes,
                        engine->session->currentIndex,
                        engine->session->accumulatedTimeMs);
    memcpy(snapshot.stats.timestamp, engine->session->cachedTimestamp, sizeof(snapshot.stats.timestamp));

    snapshot.state = engine->state;
    snapshot.stopCause = engine->stopCause;

    return snapshot;
}
