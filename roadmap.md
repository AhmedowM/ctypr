# ctypr Project Roadmap

## Project Overview

ctypr is a **C rewrite** of the C++ typing practice library [typr-cpp](https://github.com/your-username/typr-cpp). The C++ version is fully featured with content providers, SQLite persistence, chunking, logging, 62 test cases, and a complete CMake build system. This C port is in early stages — core engine logic exists but most features are not yet implemented.

**Reference C++ project location:** `C:\Users\Secondary\Projects\typr-cpp`

---

## C++ Reference Feature Map

The table below shows which features from the C++ version exist in the C port, which are partially implemented, and which are missing entirely.

| Feature | C++ Status | C Status | Notes |
|---------|-----------|----------|-------|
| Session state machine | ✅ Complete | ⚠️ Partial | Missing `ChunkFinished`, `Timeout`, `Completed` states |
| Strict & Flow modes | ✅ Complete | ✅ Implemented | Logic matches C++ |
| WPM / accuracy stats | ✅ Complete | ⚠️ Partial | Duration in seconds (C++ uses ms), no incorrect counter |
| Timeout handling | ✅ Complete | ❌ Missing | `timeout` field exists but never checked |
| Content chunking | ✅ Complete | ❌ Missing | No formatter system |
| String content provider | ✅ Complete | ❌ Missing | Hardcoded placeholder text only |
| File content provider | ✅ Complete | ❌ Missing | |
| Database content provider | ✅ Complete | ❌ Missing | |
| SQLite persistence | ✅ Complete | ❌ Missing | Empty `src/db/` directory |
| Exception hierarchy | ✅ Complete | ❌ Missing | C has error codes but no hierarchy |
| Signal/callback system | ✅ Complete | ⚠️ Partial | Per-event callbacks exist but type-unsafe, no disconnect |
| Logger | ✅ Complete | ❌ Missing | |
| Session data struct | ✅ Complete | ❌ Missing | `SessionStats` exists but no persistence struct |
| Auto-save | ✅ Complete | ❌ Missing | |
| CMake build system | ✅ Complete | ❌ Missing | Entirely commented out |
| CMake install/packaging | ✅ Complete | ❌ Missing | |
| Test suite (62 tests) | ✅ Complete | ❌ Missing | Empty `tests/` directory |
| Pimpl pattern | ✅ Complete | ⚠️ Partial | Engine struct in .c but no full Pimpl |
| Doxygen documentation | ✅ Complete | ❌ Missing | |

---

## Fixed Bugs

The following 10 bugs were identified and fixed in the initial bug-fix pass:

| # | Bug | Fix |
|---|-----|-----|
| 1 | Backspace logic inversion — `correctKeystrokes` decremented for incorrect chars | Changed condition: only decrement `correctKeystrokes` when the backspaced character was correct; reset flag for incorrect ones |
| 2 | `totalKeystrokes` incremented on backspace | Removed `self->stats.totalKeystrokes++` from `engineBackspacePress_Flow` |
| 3 | `clock()` used instead of wall-clock time | Replaced with `QueryPerformanceCounter` (Windows) / `clock_gettime(CLOCK_MONOTONIC)` (POSIX) |
| 4 | Accumulated time truncated to whole seconds | Changed to `int64_t accumulatedTimeMs` — stores milliseconds with no truncation |
| 5 | Sentinel value `0` conflicted with valid `clock()` return | Added `bool isTimingStarted` flag instead of checking `segmentStartTime == 0` |
| 6 | `ENGINE_EVENT_FINISHED` never fired on completion | Added `engineExecuteCallbacks(self, &(EngineEvent){ENGINE_EVENT_FINISHED})` in both strict and flow modes |
| 7 | `void*` event parameter in callbacks (type-unsafe) | Changed to `EngineEvent*` in `callback.h` and `engine.c`; moved `EngineCallback` typedef to `event.h` to resolve circular dependency |
| 8 | `ENGINE_EVENT_STOPPED` never fired on completion | Added `engineExecuteCallbacks(self, &(EngineEvent){ENGINE_EVENT_STOPPED})` alongside finish event |
| 9 | `strcpy` without bounds checking | Replaced with `snprintf(self->session->text, sizeof(self->session->text), ...)` |
| 10 | `totalKeystrokes` and `currentIndex` coupled in Flow mode | Moved `totalKeystrokes++` to before the correct/incorrect branch so it's always incremented once per keypress, independent of `currentIndex++` |

---

## Code Improvements

### 1. Build system activation (CMakeLists.txt)

**Problem:** The entire CMakeLists.txt is commented out. The project has no build targets and cannot be compiled. The C++ version has a complete CMake build with install, packaging, and test integration.

**Improvement:** Uncomment and configure the build system to compile `engine.c` into a static/shared library. Add proper target linkage options, install rules, and test infrastructure. Reference the C++ version's `CMakeLists.txt` for the full feature set.

### 2. Missing test suite (tests/)

**Problem:** The `tests/` directory is empty. The C++ version has 62 test cases across `test_session`, `test_providers`, and `test_repository`.

**Improvement:** Add a test framework (e.g., CMake's CTest with a simple C test runner, or a library like Unity/CMocka). Add unit tests for:
- Engine lifecycle (create, start, pause, resume, stop, reset)
- Strict mode key processing
- Flow mode key processing
- Backspace behavior in both modes
- Stats calculation (accuracy, WPM, duration)
- Callback registration and execution
- Error handling
- State machine transitions

### 3. Empty feature directories (src/content, src/db, src/format, src/utils)

**Problem:** Several directories exist but contain no files. The C++ version has full implementations for all of these.

**Improvement:** Implement the modules following the C++ version's architecture:
- `src/content/` — Content providers (String, File, Database) with factory functions
- `src/db/` — SQLite-based session persistence (Repository with CRUD + queries)
- `src/format/` — Content chunking (SentenceFormatter)
- `src/utils/` — Logger, timing utilities, platform abstraction

### 4. `ENGINE_EVENT_STARTED` callback never fired

**Location:** `src/core/engine.c`, `engineStart()` function

**Problem:** When the engine starts, no callbacks are executed to notify listeners. The C++ version fires `onSessionStart`.

**Improvement:** Add `engineExecuteCallbacks(self, &(EngineEvent){ENGINE_EVENT_STARTED})` at the end of `engineStart()`.

### 5. Engine struct defined in .c file prevents user stack allocation

**Location:** `src/core/engine.c`, line 45-60

**Problem:** The `Engine` struct is defined in the `.c` file, making it opaque to users. This is fine for encapsulation but prevents stack allocation and forces all users to use `engineCreate`/heap allocation. The C++ version uses Pimpl for the same effect.

**Improvement:** Consider exposing the struct definition (or a forward-compatible version) in the header so users can optionally stack-allocate engines for performance-sensitive applications.

### 6. Missing timeout implementation

**Location:** `src/core/engine.h` and `src/core/engine.c`

**Problem:** The engine has a `timeout` field that can be set and retrieved, but the timeout is never checked during execution. Sessions will never automatically stop due to timeout. The C++ version fully implements timeout with `ENGINE_EVENT_TIMEOUT` and a `Timeout` state.

**Improvement:** Implement timeout checking in `engineKeyPress` (or a polling mechanism), emitting `ENGINE_EVENT_TIMEOUT` when the session exceeds the configured duration. Add `ENGINE_STOP_CAUSE_TIMEOUT` handling.

### 7. No incorrect keystrokes counter in SessionStats

**Location:** `src/core/stats.h`

**Problem:** `SessionStats` has `correctKeystrokes` and `totalKeystrokes` but no explicit `incorrectKeystrokes` counter. Accuracy is calculated from these two values. With the backspace bugs, this can produce accuracy > 100% or other corrupt values. The C++ version's `Stats` struct also derives incorrect from total - correct, but handles it more carefully.

**Improvement:** Add an explicit `incorrectKeystrokes` field to `SessionStats` to simplify calculations and provide a single source of truth independent of potential counter corruption.

### 8. Missing `#include "event.h"` in callback.h

**Location:** `src/core/callback.h`, line 4

**Problem:** The include for `event.h` is commented out. While `EngineEvent` is used only via `void*` pointers, the missing include means `callback.h` doesn't properly declare its dependency.

**Improvement:** Uncomment the include or properly separate concerns so header dependencies are explicit.

### 9. NULL-pointer safety inconsistencies

**Location:** Various functions in `engine.c`

**Problem:** Some functions check for NULL `self`, some don't. For example, `engineSetMode` checks for NULL but `engineDestroy` also checks. The pattern is inconsistent.

**Improvement:** Standardize NULL checking — either validate in all functions or accept crashes on NULL as a precondition violation with a clear contract.

### 10. Use of `strcpy`, `sprintf`, and raw buffers

**Improvement:** Replace `strcpy` with `snprintf` or `strncpy` throughout. Consider using dynamic string allocation or safer buffer management utilities.

### 11. State machine missing states

**Location:** `src/core/state.h`

**Problem:** The C++ version has 8 states: `Idle`, `Running`, `Paused`, `ChunkFinished`, `Stopped`, `Timeout`, `Completed`, `Error`. The C version only has 4: `ENGINE_IDLE`, `ENGINE_RUNNING`, `ENGINE_PAUSED`, `ENGINE_ERROR`. Missing states prevent proper timeout handling, chunk-based sessions, and clean completion detection.

**Improvement:** The C version already has a separate `EngineStopCause` enum (`ENGINE_STOP_CAUSE_TIMEOUT`, `ENGINE_STOP_CAUSE_FINISHED`, etc.) which is a **better approach for C** than baking stop reasons into the state enum. Keep `EngineStopCause` as secondary metadata alongside `EngineState`. This avoids state explosion and separates "what is the engine doing?" from "why did it stop?".

Use `EngineStopCause` together with `EngineState` for full context:
- `state = ENGINE_IDLE, stopCause = ENGINE_STOP_CAUSE_TIMEOUT` → session timed out
- `state = ENGINE_IDLE, stopCause = ENGINE_STOP_CAUSE_FINISHED` → session completed
- `state = ENGINE_IDLE, stopCause = ENGINE_STOP_CAUSE_USER` → user stopped
- `state = ENGINE_RUNNING, stopCause = ENGINE_STOP_CAUSE_FINISHED` → chunk completed, preparing for next chunk

### 12. Duration should be in milliseconds

**Location:** `src/core/stats.h`, line 10

**Problem:** `SessionStats.duration` is `uint32_t` (seconds). The C++ version stores duration in milliseconds (`int64_t`), which is necessary for accurate WPM calculation and sub-second precision.

**Improvement:** Change `duration` to `uint64_t` and store milliseconds, matching the C++ version's `SessionData.durationMs`.

---

## Suggestions

### 1. Implement wall-clock timing with sub-second precision (C++ reference)

Replace the `clock()`-based timing with a cross-platform abstraction matching the C++ version's `std::chrono::steady_clock`:
- **POSIX:** `clock_gettime(CLOCK_MONOTONIC, ...)`
- **Windows:** `QueryPerformanceFrequency()` / `QueryPerformanceCounter()`
- Store accumulated time in milliseconds (uint64_t) to match `SessionData.durationMs`

### 2. Implement content provider system (C++ reference)

Port the C++ version's content provider architecture:
- **ContentProvider** — Abstract base with `getNextContent()` and `reset()`
- **StringContentProvider** — Content from a literal string
- **FileContentProvider** — Content from a text file (with optional random line selection)
- **DatabaseContentProvider** — Content from SQLite database
- **Factory functions** — `Provider::string()`, `Provider::file()`, `Provider::randomLine()`, `Provider::database()`

### 3. Implement content chunking / formatters (C++ reference)

Port the C++ version's formatter system:
- **Formatter** — Abstract base with `format(const char* text)` 
- **SentenceFormatter** — Splits text into sentence-bounded chunks with configurable max chunk size
- Support multi-chunk sessions with `ChunkFinished` state

### 4. Implement SQLite persistence (C++ reference)

Port the C++ version's Repository:
- **SessionData struct** — `id`, `timestamp`, `mode`, `totalChars`, `correctChars`, `durationMs`, `wpm`, `wpmRaw`, `accuracy`
- **CRUD operations** — `saveSession`, `getSession`, `getAll`, `getRecent`, `deleteSession`
- **Query operations** — `getSessionsByMode`, `getBestWpm`, `getBestRawWpm`, `getAverageWpm`
- **Auto-save** — Optional automatic persistence on session completion

### 5. Implement type-safe callback/signal system (C++ reference)

Port the C++ version's Signal class:
- Per-event callback lists (one list per event type) instead of iterating all callbacks
- `connect(Slot)` with disconnect token support
- `connectSimple(Slot)` without disconnect
- `emit(Args...)` to fire all connected slots
- `disconnect(token)` to remove specific connections
- `compact()` to clean up expired token slots
- `clear()` to remove all connections

### 6. Implement exception/error hierarchy (C++ reference)

Port the C++ version's exception hierarchy to C error codes:
- `ENGINE_ERROR_NONE` — No error
- `ENGINE_ERROR_CONFIG` — Configuration error (invalid mode, unknown option)
- `ENGINE_ERROR_STATE` — State machine error (invalid transition)
- `ENGINE_ERROR_PROVIDER` — Provider error (database query failure)
- `ENGINE_ERROR_CONTENT` — Content error (empty file, invalid format)
- `ENGINE_ERROR_FILE` — File I/O error (file not found, permission denied)

### 7. Implement logger (C++ reference)

Port the C++ version's Logger:
- Configurable log levels (Debug, Info, Warn, Error)
- Stdout and file output support
- Simple API: `log(level, message)`

### 8. Add auto-save functionality (C++ reference)

Port the C++ version's `setAutoSave(bool)`:
- When enabled, automatically persist session data on completion/timeout
- Integrate with the Repository module

### 9. Implement test access macros (C++ reference)

Port the C++ version's `TYPR_ENABLE_TEST_ACCESS` pattern:
- Conditionally expose internal state for testing
- `getContent()`, `getIndex()`, `getChunkCount()`, `getChunkIndex()`
- `setFormatter()` for testing different formatters

### 10. Add continuous integration

Set up GitHub Actions (or similar) to:
- Build on Windows, macOS, and Linux
- Run the test suite on every push/PR
- Check code formatting with clang-format
- Run static analysis (cppcheck, clang-tidy)

### 11. Optimize memory usage

The `incorrectKeystrokesIndices` array allocates 4096 × `uint16_t` = 8 KB per session. Consider:
- Using a bitset instead of `uint16_t` values (4096 bits = 512 bytes)
- Making the buffer size configurable or dynamic
- Using a sparse representation for long texts

### 12. Create proper API documentation

Add comprehensive Doxygen-style documentation matching the C++ version:
- Document all public functions with `@param`, `@return`, `@see` tags
- Provide usage examples in header comments
- Generate HTML documentation as part of the build

---

## Implementation Priority

### ✅ Phase 1 — Core Bug Fixes (completed)
The following bugs have been fixed in the initial bug-fix pass:
- Backspace logic inversion (Bug #1) — fixed
- `totalKeystrokes` inflation on backspace (Bug #2) — fixed
- `clock()` replaced with wall-clock timing (Bug #3) — fixed
- Accumulated time truncation (Bug #4) — fixed
- Sentinel value conflict (Bug #5) — fixed
- Missing finish event (Bug #6) — fixed
- Type-unsafe `void*` in callbacks (Bug #7) — fixed
- Missing stopped event (Bug #8) — fixed
- `strcpy` overflow risk (Bug #9) — fixed
- Flow mode keystroke tracking (Bug #10) — fixed
- `ENGINE_EVENT_STARTED` callback (Improvement #4) — fixed
- Header dependency cleanup (Improvement #8) — fixed
- Duration changed to milliseconds (Improvement #12) — fixed

### Phase 2 — Build & Test Infrastructure
- Build system activation (Improvement #1)
- Add test suite (Improvement #2)

### Phase 3 — Feature Parity with C++ Version
- Implement timeout handling (Improvement #6)
- Implement content provider system (Suggestion #2)
- Implement content chunking/formatters (Suggestion #3)
- Implement SQLite persistence (Suggestion #4)
- Implement type-safe signal system (Suggestion #5)
- Implement logger (Suggestion #7)
- Implement auto-save (Suggestion #8)

### Phase 4 — Polish & Quality
- Error hierarchy expansion (Suggestion #6)
- Test access macros (Suggestion #9)
- Incorrect keystrokes counter (Improvement #7)
- NULL-check consistency (Improvement #9)
- Memory optimization (Suggestion #11)
- API documentation (Suggestion #12)
- Continuous integration (Suggestion #10)
- Engine struct visibility (Improvement #5)
- State machine states (Improvement #11)
