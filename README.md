# ctypr

A C library for typing sessions. It tracks speed, accuracy, and timing — embed it in tutors, CLI tools, or games.

## What it does

- **Two modes**: strict (blocks on wrong keys) or flow (advances anyway, supports backspace)
- **Stats**: WPM (raw and accuracy-adjusted), keystroke counts, elapsed time
- **Content from anywhere**: strings, files, SQLite databases, or web URLs
- **Auto-save**: sessions persist to SQLite on finish, timeout, or manual stop
- **Callbacks**: hook into started, stopped, keystrokes, backspace, timeout, segment done
- **Snapshot**: one call gets the full session state for rendering
- **Logger**: stdout or file, configurable levels
- **Zero deps beyond C17**: SQLite downloads and builds via CMake

## Build

```sh
cmake -B build
cmake --build build
```

Tests and examples build by default:

```sh
cmake -B build -DBUILD_TESTING=ON -DBUILD_EXAMPLES=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Minimal example

```c
#include "engine.h"
#include "content.h"
#include "snapshot.h"
#include <stdio.h>

int main(void) {
    ContentProvider* cp = contentProviderFromString(
        "The quick brown fox jumps over the lazy dog."
    );
    if (!cp) return 1;

    Engine* e = engineCreate(&(EngineConfig){
        .mode = FlowMode,
        .timeout = 60,
        .contentProvider = cp,
    });
    if (!e) { contentProviderDestroy(cp); return 1; }

    engineStart(e);
    for (const char* p = "The quick brown fox jumps over the lazy dog."; *p; p++) {
        engineKeyPress(e, *p);
        if (engineIsCompleted(e)) break;
    }

    EngineSnapshot snap = engineGetSnapshot(e);
    printf("WPM: %.1f  Accuracy: %.1f%%\n", snap.stats.wpm, snap.stats.accuracy);

    engineDestroy(e);
    contentProviderDestroy(cp);
    return 0;
}
```

## API

### Engine

```c
Engine* engineCreate(const EngineConfig* config);
void engineDestroy(Engine* self);

void engineSetLogger(Engine* self, Logger* logger);
void engineSetContentProvider(Engine* self, ContentProvider* provider);
void engineSetAutoSave(Engine* self, Repository* repo, bool enabled);
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

### Snapshot

```c
EngineSnapshot engineGetSnapshot(Engine* engine);
// Fields: text[4096], length, cursorIndex, expectedChar,
//         incorrectFlags[4096], stats (see below),
//         state, stopCause
```

### Stats

```c
SessionStats engineGetStats(Engine* engine);
// Fields: timestamp[20], durationMs,
//         correctKeystrokes, incorrectKeystrokes, totalKeystrokes,
//         accuracy, wpm, wpmRaw
```

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

### Content providers

```c
ContentProvider* contentProviderFromString(const char* text);
ContentProvider* contentProviderFromFile(const char* filepath);
ContentProvider* contentProviderFromDatabase(const char* filepath);
ContentProvider* contentProviderFromWeb(const char* url);
void contentProviderDestroy(ContentProvider* provider);

void contentProviderSetMode(ContentProvider* self, ContentMode mode);
// CONTENT_MODE_SENTENCES, CONTENT_MODE_COMMON_WORDS, CONTENT_MODE_RANDOM_WORDS

void contentProviderSetContentLimit(ContentProvider* self, size_t limit);
void contentProviderSetLogger(ContentProvider* self, Logger* logger);
ContentChunk contentProviderGetNext(ContentProvider* provider);
bool contentProviderIsExhausted(ContentProvider* provider);
void contentProviderReset(ContentProvider* provider);
```

### Repository (SQLite)

```c
Repository* repositoryCreate(const char* dbPath);
void repositoryDestroy(Repository* repo);
void repositorySetLogger(Repository* self, Logger* logger);

int64_t repositorySaveSession(Repository* repo, const SessionData* data);
SessionData repositoryGetSession(Repository* repo, int64_t id);
SessionData* repositoryGetAll(Repository* repo, size_t* count);
SessionData* repositoryGetRecent(Repository* repo, int64_t limit, size_t* count);
int64_t repositoryGetCount(Repository* repo);
bool repositoryDeleteSession(Repository* repo, int64_t id);
void repositoryClearAll(Repository* repo);
SessionData repositoryGetBestWpm(Repository* repo);
SessionData repositoryGetBestRawWpm(Repository* repo);
double repositoryGetAverageWpm(Repository* repo);
```

### Logger

```c
Logger* loggerCreate(LogLevel level, bool enableStdout);
void loggerDestroy(Logger* logger);
void loggerSetLevel(Logger* logger, LogLevel level);
LogLevel loggerGetLevel(Logger* logger);
void loggerLog(Logger* logger, LogLevel level, const char* message);
bool loggerAddFile(Logger* logger, const char* filepath);
void loggerLogToStdout(Logger* logger, bool enable);
```

### Formatter

```c
Formatter* formatterCreate(void);
void formatterDestroy(Formatter* formatter);
void formatterSetLogger(Formatter* self, Logger* logger);
void formatterReset(Formatter* self);
ContentChunk formatterFormat(Formatter* self, const char* text, size_t maxChunkSize);
```

### Version

```c
#include "version.h"
// CTYPR_VERSION_MAJOR, CTYPR_VERSION_MINOR, CTYPR_VERSION_PATCH
// CTYPR_VERSION_STRING
```

### Helpers

```c
void engineErrorToString(EngineError error, char* buffer, size_t bufferSize);
void engineEventToString(EngineEvent event, char* buffer, size_t bufferSize);
```

## Layout

```
src/
  core/     engine, state, stats, snapshot, signals, errors, version
  content/  string, file, database, web providers
  format/   text chunking
  db/       SQLite layer
  utils/    logger
tests/      5 suites (engine, content, formatter, repository, logger)
examples/   usage.c — full API walkthrough
```

## Run tests

```sh
ctest --test-dir build --output-on-failure
```

Or individually:

```sh
./build/tests/test_engine
./build/tests/test_content
./build/tests/test_formatter
./build/tests/test_repository
./build/tests/test_logger
```