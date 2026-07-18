ctypr {#mainpage}
=====

A small C library that simulates typing sessions and measures speed and accuracy.

## Quick start

```c
// Include modules individually:

// #include "ctypr/engine.h"
// #include "ctypr/content.h"
// #include "ctypr/stats.h"

// Or everything at once:

#include "ctypr.h"

int main(void) {
    ContentProvider* cp = contentProviderFromString(
        "The quick brown fox jumps over the lazy dog.");
    Engine* e = engineCreate(&(EngineConfig){
        .mode = FlowMode,
        .timeout = 60,
        .contentProvider = cp,
    });
    engineStart(e);
    // type the text ...
    SessionStats s = engineGetStats(e);
    printf("WPM: %.1f  Accuracy: %.1f%%\n", s.wpm, s.accuracy);
    engineDestroy(e);
    contentProviderDestroy(cp);
}
```

## Features

- **Two typing modes** — strict (locked advance) and flow (backspace-supported)
- **WPM, accuracy, timing** — adjusted and raw WPM from wall-clock time
- **Multiple content sources** — strings, files, SQLite databases
- **SQLite persistence** — auto-save on completion/timeout/stop
- **Event callbacks** — signals for start, stop, keystroke, backspace, error
- **Logger** — stdout and file output with configurable levels
- **Zero network dependencies** — SQLite fetched at build time, everything else is C17

## Modules

| Module | Header | Purpose |
|---|---|---|
| Engine | @ref engine.h | Typing session lifecycle, keystroke processing, state machine |
| Callback | @ref callback.h | Event registration and disconnection |
| Content | @ref content.h | String, file, database, and web content providers |
| Stats | @ref stats.h | Session statistics (WPM, accuracy, keystrokes) |
| State | @ref state.h | Engine state and stop cause enums |
| Error | @ref error.h | Error codes and string conversion |
| Event | @ref event.h | Event types and callback typedef |
| Repository | @ref repository.h | SQLite session CRUD and queries |
| Logger | @ref logger.h | Configurable diagnostic logging |
| Formatter | @ref formatter.h | Text chunking (used internally by file provider) |

## Building

```sh
cmake -B build
cmake --build build              # library only
cmake -B build -DBUILD_TESTING=ON
cmake --build build              # library + tests
cmake -B build -DBUILD_DOCS=ON
cmake --build build --target docs  # library + HTML docs
```

## Testing

```sh
cmake -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```
