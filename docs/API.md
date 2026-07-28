# ctypr API Reference

## Engine

```c
Engine* engineCreate(const EngineConfig* config);
void engineDestroy(Engine* self);

void engineSetLogger(Engine* self, Logger* logger);
void engineSetContentProvider(Engine* self, ContentProvider* provider);
void engineSetAutoSave(Engine* self, Repository* repo, bool enabled);
```

### Config

```c
typedef struct EngineConfig {
    EngineMode mode;                  // StrictMode or FlowMode (required)
    uint16_t timeout;                 // seconds, 0 = no limit
    ContentProvider* contentProvider; // required
    Repository* autoSaveRepo;         // optional
    bool autoSaveEnabled;
} EngineConfig;
```

### Mode & timeout

```c
void engineSetMode(Engine* self, EngineMode mode);
EngineMode engineGetMode(Engine* self);
void engineSetTimeout(Engine* self, uint16_t timeout);
uint16_t engineGetTimeout(Engine* self);
```

### Session control

```c
void engineStart(Engine* self);
void engineStop(Engine* self);
void enginePause(Engine* self);
void engineResume(Engine* self);
void engineReset(Engine* self);
void engineTick(Engine* self);  // advance timer manually (game loops)
```

### State queries

```c
EngineStateInfo engineGetStateInfo(Engine* engine);
bool engineIsRunning(Engine* self);
bool engineIsPaused(Engine* self);
bool engineIsIdle(Engine* self);
bool engineIsError(Engine* self);
bool engineIsCompleted(Engine* self);
bool engineIsTimedOut(Engine* self);
bool engineIsStopped(Engine* self);
bool engineWasStopped(Engine* self);
```

### Keystrokes

```c
void engineKeyPress(Engine* self, char key);
void engineBackspacePress(Engine* self);  // flow mode only
```

### Snapshot

```c
EngineSnapshot engineGetSnapshot(Engine* engine);
```

`EngineSnapshot` packs the full UI-renderable state:

| Field | Type | Description |
|-------|------|-------------|
| `text` | `char[4096]` | Current session content |
| `length` | `size_t` | Content length |
| `cursorIndex` | `uint32_t` | Cursor position |
| `expectedChar` | `char` | Character at cursor |
| `incorrectFlags` | `bool[4096]` | Per-position incorrect flags |
| `stats` | `SessionStats` | Live stats (see below) |
| `state` | `EngineState` | Current engine state |
| `stopCause` | `EngineStopCause` | Last stop reason |

### Stats

```c
SessionStats engineGetStats(Engine* engine);
```

| Field | Type | Description |
|-------|------|-------------|
| `timestamp` | `char[20]` | Session start time |
| `durationMs` | `int64_t` | Elapsed milliseconds |
| `correctKeystrokes` | `uint32_t` | Correct key presses |
| `incorrectKeystrokes` | `uint32_t` | Incorrect key presses |
| `totalKeystrokes` | `uint32_t` | Total key presses |
| `accuracy` | `double` | Accuracy percentage |
| `wpm` | `double` | Adjusted WPM (accuracy × raw) |
| `wpmRaw` | `double` | Raw WPM |

### Signals

```c
int engineOnStarted(Engine* engine, EngineCallback cb, void* data);
int engineOnStopped(Engine* engine, EngineCallback cb, void* data);
int engineOnFinished(Engine* engine, EngineCallback cb, void* data);
int engineOnTimeout(Engine* engine, EngineCallback cb, void* data);
int engineOnPaused(Engine* engine, EngineCallback cb, void* data);
int engineOnResumed(Engine* engine, EngineCallback cb, void* data);
int engineOnCorrectKeystroke(Engine* engine, EngineCallback cb, void* data);
int engineOnIncorrectKeystroke(Engine* engine, EngineCallback cb, void* data);
int engineOnBackspace(Engine* engine, EngineCallback cb, void* data);
int engineOnSegmentCompleted(Engine* engine, EngineCallback cb, void* data);
int engineOnError(Engine* engine, EngineCallback cb, void* data);

void engineDisconnect(Engine* engine, EngineEvent event, int slotId);
void engineClearEvent(Engine* engine, EngineEvent event);
```

### Error / Event strings

```c
void engineErrorToString(EngineError error, char* buffer, size_t bufferSize);
void engineEventToString(EngineEvent event, char* buffer, size_t bufferSize);
```

---

## Content Providers

```c
ContentProvider* contentProviderFromString(const char* text);
ContentProvider* contentProviderFromFile(const char* filepath);
ContentProvider* contentProviderFromDatabase(const char* filepath);
ContentProvider* contentProviderFromWeb(const char* url);
void contentProviderDestroy(ContentProvider* provider);
```

### Modes

```c
void contentProviderSetMode(ContentProvider* self, ContentMode mode);
```

| Mode | Source table | Use case |
|------|-------------|----------|
| `CONTENT_MODE_SENTENCES` | `typing_sentences` | Full sentences for paragraph practice |
| `CONTENT_MODE_COMMON_WORDS` | `words` (ORDER BY frequency_rank) | Most frequent words first |
| `CONTENT_MODE_RANDOM_WORDS` | `words` (ORDER BY RANDOM()) | Random word drill |

### Configuration

```c
void contentProviderSetContentLimit(ContentProvider* self, size_t limit);
void contentProviderSetDifficultyFilter(ContentProvider* self, const char* difficulty);
void contentProviderSetWordLengthRange(ContentProvider* self, size_t min_len, size_t max_len);
void contentProviderSetLogger(ContentProvider* self, Logger* logger);
```

- **Content limit**: maximum rows per query (default 300)
- **Difficulty filter**: `"Easy"`, `"Normal"`, `"Hard"`, `"Expert"`, or NULL to clear (sentences only)
- **Word length range**: inclusive min/max, `0` = no bound (words only)
- Filters persist across `contentProviderReset()`

### Access

```c
ContentChunk contentProviderGetNext(ContentProvider* provider);
bool contentProviderIsExhausted(ContentProvider* provider);
void contentProviderReset(ContentProvider* provider);
```

```c
typedef struct ContentChunk {
    char text[4096];
    size_t length;
} ContentChunk;
```

---

## Repository (SQLite)

```c
Repository* repositoryCreate(const char* dbPath);
void repositoryDestroy(Repository* repo);
void repositorySetLogger(Repository* self, Logger* logger);

int64_t repositorySaveSession(Repository* repo, const SessionData* data);
SessionData repositoryGetSession(Repository* repo, int64_t id);
SessionData* repositoryGetAll(Repository* repo, size_t* count);
SessionData* repositoryGetRecent(Repository* repo, int64_t limit, size_t* count);
SessionData* repositoryGetSessionsByMode(Repository* repo, const char* mode, size_t* count);
int64_t repositoryGetCount(Repository* repo);
bool repositoryDeleteSession(Repository* repo, int64_t id);
void repositoryClearAll(Repository* repo);
SessionData repositoryGetBestWpm(Repository* repo);
SessionData repositoryGetBestRawWpm(Repository* repo);
double repositoryGetAverageWpm(Repository* repo);
```

`SessionData` fields: `id`, `timestamp[20]`, `mode[16]`, `totalChars`, `correctChars`, `durationMs`, `wpm`, `wpmRaw`, `accuracy`.

Default database path: `"typr.db"` (in current directory) when NULL is passed to `repositoryCreate`.

---

## Logger

```c
Logger* loggerCreate(LogLevel level, bool enableStdout);
void loggerDestroy(Logger* logger);
void loggerSetLevel(Logger* logger, LogLevel level);
LogLevel loggerGetLevel(Logger* logger);
void loggerLog(Logger* logger, LogLevel level, const char* message);
bool loggerAddFile(Logger* logger, const char* filepath);
void loggerLogToStdout(Logger* logger, bool enable);
```

### Levels

`LOG_LEVEL_DEBUG` < `LOG_LEVEL_INFO` < `LOG_LEVEL_WARNING` < `LOG_LEVEL_ERROR` < `LOG_LEVEL_NONE`

Messages below the configured level are suppressed. Logger is thread-safe (mutex-guarded).

---

## Formatter

```c
Formatter* formatterCreate(void);
void formatterDestroy(Formatter* formatter);
void formatterSetLogger(Formatter* self, Logger* logger);
void formatterReset(Formatter* self);
ContentChunk formatterFormat(Formatter* self, const char* text, size_t maxChunkSize);
```

Internal use by the file content provider for sentence-boundary chunking.

---

## Version

```c
#include "ctypr/version.h"
// CTYPR_VERSION_MAJOR, CTYPR_VERSION_MINOR, CTYPR_VERSION_PATCH
// CTYPR_VERSION_STRING
```
