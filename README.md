# ctypr

[![CI](https://github.com/AhmedowM/ctypr/actions/workflows/ci.yml/badge.svg)](https://github.com/AhmedowM/ctypr/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/AhmedowM/ctypr)](https://github.com/AhmedowM/ctypr/releases)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-linux%20|%20macOS%20|%20windows-blue)]()
[![C17](https://img.shields.io/badge/C_Standard-C17-blue)]()

A C17 library for typing sessions — speed, accuracy, timing. Embed it in tutors, CLI tools, or games.

- **Two modes**: strict (blocks on wrong keys) or flow (advances anyway, backspace allowed)
- **Stats**: WPM (raw + accuracy-adjusted), keystroke counts, elapsed time
- **Content**: strings, files, SQLite databases, or web URLs (stub — v2 feature)
- **Auto-save**: sessions persist to SQLite on finish/timeout/stop
- **Callbacks**: 11 event hooks — started, stopped, keystrokes, backspace, timeout, segment done, error
- **Snapshot**: single call gets full session state (text, cursor, incorrect flags, stats) for rendering
- **Logger**: stdout and/or file, configurable levels, thread-safe
- **Content DB pipeline**: Python scripts (in `tools/content-db/`) to build `sentences.db` and `words.db`
- **Zero runtime deps**: SQLite downloads and builds via CMake

---

## Table of Contents

- [Prerequisites](#prerequisites)
- [Quickstart](#quickstart)
- [Build & Install](#build--install)
- [Examples](#examples)
  - [Minimal session (string)](#minimal-session)
  - [Database content](#database-content)
  - [Auto-save with repository](#auto-save-with-repository)
- [API Highlights](#api-highlights)
- [Project Layout](#project-layout)
- [Tests](#tests)
- [License](#license)

---

## Prerequisites

- **Compiler**: C17-capable (GCC 10+, Clang 12+, MSVC 2022 17.0+)
- **CMake**: 3.25+
- **Platforms**: Linux, macOS, Windows (CI-tested on all three)
- **Network**: First build downloads SQLite amalgamation automatically; no other network access needed

## Quickstart

```sh
cmake -B build
cmake --build build
./build/examples/usage          # Linux/macOS
.\build\examples\usage.exe      # Windows
```

That builds the library, all 5 test suites, and the full API walkthrough example in one command.

## Build & Install

```sh
cmake -B build                 # configure
cmake --build build            # build library + tests + examples
ctest --test-dir build         # run all tests (optional)
cmake --install build --prefix /path/to/install  # install
```

### Consuming in another project

```cmake
# CMakeLists.txt
find_package(ctypr REQUIRED)
target_link_libraries(my_app PRIVATE ctypr::ctypr)
```

Or embed the source directly — no external dependencies beyond a C17 compiler and CMake.

### Build options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_TESTING` | `ON` | Build test executables |
| `BUILD_EXAMPLES` | `ON` | Build example executable |

```sh
cmake -B build -DBUILD_TESTING=ON -DBUILD_EXAMPLES=ON
```

## Examples

Full walkthrough: [`examples/usage.c`](examples/usage.c)

### Minimal session

Creates a string content provider, types every character, reads the snapshot:

```c
#include "ctypr.h"
#include <stdio.h>

int main(void) {
    ContentProvider* cp = contentProviderFromString(
        "The quick brown fox jumps over the lazy dog."
    );
    Engine* e = engineCreate(&(EngineConfig){
        .mode = FlowMode, .timeout = 60, .contentProvider = cp,
    });

    engineStart(e);
    for (const char* p = "The quick brown fox jumps over the lazy dog."; *p; p++) {
        engineKeyPress(e, *p);
    }

    EngineSnapshot snap = engineGetSnapshot(e);
    printf("WPM: %.1f  Accuracy: %.1f%%\n", snap.stats.wpm, snap.stats.accuracy);
    // Output: WPM: ~154.2  Accuracy: ~100.0%

    engineDestroy(e);
    contentProviderDestroy(cp);
    return 0;
}
```

### Database content

Loads words from a SQLite database built by the [pipeline](tools/content-db/):

```c
ContentProvider* cp = contentProviderFromDatabase("words.db");
contentProviderSetMode(cp, CONTENT_MODE_COMMON_WORDS);
contentProviderSetContentLimit(cp, 200);
contentProviderSetWordLengthRange(cp, 4, 8);

Engine* e = engineCreate(&(EngineConfig){
    .mode = StrictMode, .timeout = 30, .contentProvider = cp,
});
// ... session runs with 4-8 letter words, most frequent first
```

### Auto-save with repository

```c
Repository* repo = repositoryCreate("sessions.db");
ContentProvider* cp = contentProviderFromString("Practice text.");

Engine* e = engineCreate(&(EngineConfig){
    .mode = FlowMode, .timeout = 60,
    .contentProvider = cp,
    .autoSaveRepo = repo,
    .autoSaveEnabled = true,
});
// Session is auto-saved on finish, timeout, or manual stop
```

## API Highlights

Full reference: [`docs/API.md`](docs/API.md)

```c
// Engine lifecycle
Engine* engineCreate(const EngineConfig* config);
void engineDestroy(Engine* self);

// Logger auto-propagates to content provider and repository
void engineSetLogger(Engine* self, Logger* logger);

// Session control
void engineStart(Engine* self);   // auto-advance timer in loops: engineTick()
void engineStop(Engine* self);
void enginePause(Engine* self);
void engineResume(Engine* self);
void engineReset(Engine* self);

// Keystrokes
void engineKeyPress(Engine* self, char key);
void engineBackspacePress(Engine* self);  // flow mode only

// Full UI state in one call
EngineSnapshot engineGetSnapshot(Engine* engine);

// Content providers
ContentProvider* contentProviderFromString(const char* text);
ContentProvider* contentProviderFromFile(const char* filepath);
ContentProvider* contentProviderFromDatabase(const char* filepath);
ContentProvider* contentProviderFromWeb(const char* url);  // stub — v2
void contentProviderSetMode(cp, CONTENT_MODE_SENTENCES|COMMON_WORDS|RANDOM_WORDS);
void contentProviderSetDifficultyFilter(cp, "Easy"|"Normal"|"Hard"|"Expert"|NULL);
void contentProviderSetWordLengthRange(cp, min_len, max_len);

// SQLite persistence
Repository* repositoryCreate(const char* dbPath);
int64_t repositorySaveSession(Repository* repo, const SessionData* data);
SessionData* repositoryGetSessionsByMode(Repository* repo, const char* mode, size_t* count);
```

## Project Layout

```
src/
  core/       engine, state, stats, snapshot, signals, errors, version
  content/    string, file, database, web providers
  format/     text chunking
  db/         SQLite repository
  utils/      logger
tests/        6 test suites (engine, content, formatter, repository, logger, concurrency)
examples/     usage.c — full API walkthrough (callbacks, signals, stats, DB)
tools/        Content DB pipeline (Python scripts)
docs/         Doxygen-generated API docs (GitHub Pages), markdown reference
```

## Tests

```sh
ctest --test-dir build --output-on-failure
```

Or individually:

```sh
./build/tests/test_engine       # 51 tests
./build/tests/test_content      # 19 tests
./build/tests/test_formatter    # 8 tests
./build/tests/test_repository   # 12 tests
./build/tests/test_logger       # 15 tests
```

**98 tests total** — across all 6 suites (engine, content, formatter, repository, logger, concurrency).

## Usage from downstream projects

Projects using ctypr can find the content DB pipeline scripts via the `CTYPR_CONTENT_DB_DIR` CMake variable (set by `find_package(ctypr)`):

```cmake
find_package(ctypr REQUIRED)
# CTYPR_CONTENT_DB_DIR points to installed scripts
# Build words.db / sentences.db at build time:
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/words.db ${CMAKE_BINARY_DIR}/sentences.db
    COMMAND ${Python3_EXECUTABLE} ${CTYPR_CONTENT_DB_DIR}/run_pipeline.py
        --sentences-db ${CMAKE_BINARY_DIR}/sentences.db
        --words-db ${CMAKE_BINARY_DIR}/words.db
    DEPENDS ${CTYPR_CONTENT_DB_DIR}/run_pipeline.py
)
```

Pre-built databases are also attached to each [GitHub release](https://github.com/AhmedowM/ctypr/releases) for convenience.

## License

Distributed under the MIT License. See [`LICENSE`](LICENSE).
