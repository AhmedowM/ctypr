# ctypr

A small C library that simulates typing sessions and measures your speed and accuracy — useful for building typing tutors, CLI games, or keyboard training tools.

## Why this exists

Most typing practice software is either a full GUI app or a quick online tool. There isn't much out there if you want to embed typing logic into your own program. ctypr gives you a clean, reusable C library that handles the session state machine, keystroke processing, timing, and persistence, so you can focus on the UI.

- **Two typing modes** — strict mode locks you until you hit the right key; flow mode keeps advancing and lets you backspace
- **WPM, accuracy, and timing** — computes raw and adjusted WPM from actual elapsed wall-clock time
- **Multiple content sources** — strings, files, or SQLite databases, with a formatter that splits text into sentence or word chunks
- **SQLite persistence** — sessions save automatically on completion, timeout, or stop; query best WPM, recent sessions, averages
- **Signal system** — per-event callbacks for started, stopped, finished, timeout, keystrokes, and backspace
- **Logger** — stdout and file output with configurable levels, useful for debugging without printf
- **Zero server or network dependencies** — SQLite is fetched and built by CMake, everything else is standard C17

## Prerequisites

- **CMake >= 3.25**
- A **C17 compiler** (GCC, Clang, MSVC — all work)
- Windows, macOS, or Linux
- An internet connection on first build (CMake downloads SQLite automatically)

## Building

```sh
git clone https://github.com/AhmedowM/ctypr.git
cd ctypr
cmake -B build
cmake --build build
```

Tests and the usage example are built by default:

```sh
cmake -B build -DBUILD_TESTING=ON -DBUILD_EXAMPLES=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## A minimal example

```c
#include "engine.h"
#include "content.h"
#include "stats.h"
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

    SessionStats s = engineGetStats(e);
    printf("WPM: %.1f  Accuracy: %.1f%%\n", s.wpm, s.accuracy);

    engineDestroy(e);
    contentProviderDestroy(cp);
    return 0;
}
```

## Current status

ctypr is functionally complete for basic typing sessions. The core engine, content providers (string, file, database), formatter, SQLite persistence, signal system, and logger are all implemented and tested (91 tests across 5 suites).

**What's left:**
- Full API documentation with Doxygen
- Type-safe disconnect tokens for the signal system
- A working web content provider (currently falls back to a default string)
- CMake install/packaging polish

## API reference

### Engine

```c
Engine* engineCreate(const EngineConfig* config);
void engineDestroy(Engine* self);

void engineSetLogger(Engine* self, Logger* logger);
void engineSetContentProvider(Engine* self, ContentProvider* provider);
void engineSetAutoSave(Engine* self, Repository* repo, bool enabled);

void engineSetMode(Engine* self, EngineMode mode);
EngineMode engineGetMode(Engine* self);
void engineSetTimeout(Engine* self, uint16_t timeout);
uint16_t engineGetTimeout(Engine* self);

void engineStart(Engine* self);
void engineStop(Engine* self);
void enginePause(Engine* self);
void engineResume(Engine* self);
void engineReset(Engine* self);

bool engineIsRunning(Engine* self);
bool engineIsPaused(Engine* self);
bool engineIsIdle(Engine* self);
bool engineIsError(Engine* self);
bool engineIsCompleted(Engine* self);
bool engineIsTimedOut(Engine* self);
bool engineIsStopped(Engine* self);
bool engineWasStopped(Engine* self);

void engineKeyPress(Engine* self, char key);
void engineBackspacePress(Engine* self);
```

### EngineConfig

```c
typedef struct EngineConfig {
    EngineMode mode;                  // StrictMode or FlowMode (required)
    uint16_t timeout;                 // seconds, 0 = no limit
    ContentProvider* contentProvider; // required, engineCreate fails if NULL
    Repository* autoSaveRepo;         // optional, NULL to disable auto-save
    bool autoSaveEnabled;
} EngineConfig;
```

### Stats

```c
SessionStats engineGetStats(Engine* engine);

// SessionStats fields:
//   char timestamp[20]       ISO 8601 snapshot time
//   int64_t durationMs       elapsed wall-clock time
//   uint32_t correctKeystrokes
//   uint32_t incorrectKeystrokes
//   uint32_t totalKeystrokes
//   double accuracy          percentage
//   double wpm               adjusted for accuracy
//   double wpmRaw            raw keystrokes / 5 / minutes
```

### State info

```c
EngineStateInfo engineGetStateInfo(Engine* engine);

// EngineStateInfo fields:
//   EngineState state         IDLE, RUNNING, PAUSED, ERROR
//   EngineStopCause stopCause NONE, TIMEOUT, FINISHED, USER, ERROR
```

### Content providers

```c
ContentProvider* contentProviderFromString(const char* text);
ContentProvider* contentProviderFromFile(const char* filepath);
ContentProvider* contentProviderFromDatabase(const char* filepath);
ContentProvider* contentProviderFromWeb(const char* url);
void contentProviderDestroy(ContentProvider* provider);

void contentProviderSetMode(ContentProvider* self, ContentMode mode);
// Modes: CONTENT_MODE_SENTENCES, CONTENT_MODE_COMMON_WORDS, CONTENT_MODE_RANDOM_WORDS

void contentProviderSetContentLimit(ContentProvider* self, size_t limit);
void contentProviderSetLogger(ContentProvider* self, Logger* logger);
ContentChunk contentProviderGetNext(ContentProvider* provider);
bool contentProviderIsExhausted(ContentProvider* provider);
void contentProviderReset(ContentProvider* provider);
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

void engineDisconnect(Engine* engine, EngineEvent event, int slotId);
void engineClearEvent(Engine* engine, EngineEvent event);
```

### Repository (SQLite session storage)

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
Logger* loggerCreate(LogLevel level, bool useStdout);
void loggerDestroy(Logger* logger);
void loggerSetLevel(Logger* logger, LogLevel level);
LogLevel loggerGetLevel(Logger* logger);
void loggerLog(Logger* logger, LogLevel level, const char* message);
bool loggerAddFile(Logger* logger, const char* filepath);
void loggerSetStdout(Logger* logger, bool enable);
```

### Formatter

```c
Formatter* formatterCreate(void);
void formatterDestroy(Formatter* formatter);
void formatterSetLogger(Formatter* self, Logger* logger);
void formatterReset(Formatter* self);
ContentChunk formatterFormat(Formatter* self, const char* text, size_t maxChunkSize);
```

### Error and event string helpers

```c
void engineErrorToString(EngineError error, char* buffer, size_t bufferSize);
void engineEventToString(EngineEvent event, char* buffer, size_t bufferSize);
```

## Project structure

```
src/
  core/     engine, state machine, stats, signals, error codes, version
  content/  string, file, database, and web content providers
  format/   text chunking / formatter
  db/       SQLite persistence layer
  utils/    logger
tests/       5 suites: test_engine, test_content, test_formatter, test_repository, test_logger
examples/    usage.c — walks through every major API
```

## Running the tests

```sh
ctest --test-dir build --output-on-failure
```

Or run a specific one:

```sh
./build/tests/test_engine
./build/tests/test_content
./build/tests/test_formatter
./build/tests/test_repository
./build/tests/test_logger
```
