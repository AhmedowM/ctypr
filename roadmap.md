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

## Bugs

### 1. Incorrect backspace logic in `engineBackspacePress_Flow` (engine.c:230-233)

**Location:** `src/core/engine.c`, lines 230-233

**Problem:** The `incorrectKeystrokesIndices` array is set to `1` at the current position when a keystroke is **incorrect**. The backspace logic checks `if (*is_correct == 1)` to decide whether to decrement `correctKeystrokes`. This logic is **inverted** — when `*is_correct == 1`, the original keypress was incorrect, so `correctKeystrokes` should NOT be decremented (it was never incremented for that press). Decrementing it here will corrupt the accuracy calculation.

**Fix:** Change the condition to check if the backspaced character was correct, or better, track per-character correctness properly. Additionally, the `totalKeystrokes` increment on backspace (line 235) should be removed — backspace is navigation, not a typographical keystroke.

### 2. `engineBackspacePress_Flow` increments `totalKeystrokes` for backspace actions (engine.c:235)

**Location:** `src/core/engine.c`, line 235

**Problem:** Backspace is an editing action, not a typing keystroke. Incrementing `totalKeystrokes` here inflates the keystroke count and reduces accuracy incorrectly.

**Fix:** Remove `self->stats.totalKeystrokes++` from `engineBackspacePress_Flow`, or separate navigation actions from typing keystrokes in stats tracking.

### 3. `clock()` is inappropriate for session timing (engine.c:36-41, 116, 155)

**Location:** `src/core/engine.c`, `updateTime()` function and all `clock()` calls

**Problem:** The engine uses `clock()` to measure session duration. `clock()` returns **processor time** (CPU time used by the process), not wall-clock time. During a typing session, when the user pauses to think (and no CPU is consumed), `clock()` will not advance. This means the session timer will under-report actual elapsed time. The C++ version uses `std::chrono::steady_clock` which is wall-clock time.

**Fix:** Use platform-specific wall-clock timing functions (e.g., `clock_gettime()` with `CLOCK_MONOTONIC` on POSIX, `QueryPerformanceCounter()` on Windows) or standard `time()` with sub-second precision.

### 4. Accumulated time truncation causes precision loss (engine.c:39)

**Location:** `src/core/engine.c`, line 39

**Problem:** `clock_t` values are divided by `CLOCKS_PER_SEC` and then added to accumulated time as whole seconds. This truncates sub-second time on every call to `updateTime()`, causing accumulated error. The C++ version stores duration in milliseconds.

**Fix:** Store accumulated time as a floating-point value or in clock ticks, converting to seconds only for display/output.

### 5. Sentinel value `0` for `segmentStartTime` conflicts with valid `clock()` return (engine.c:37)

**Location:** `src/core/engine.c`, lines 37, 117, 167

**Problem:** The code uses `segmentStartTime == 0` to check if timing has started. However, `clock()` can legally return `0` (e.g., the very first call on some systems). This would cause the timer check to incorrectly skip time updates.

**Fix:** Use a dedicated boolean flag (`isTimingStarted`) instead of relying on a sentinel value.

### 6. `engineKeyPress_Strict` doesn't fire finish event on completion (engine.c:176-179)

**Location:** `src/core/engine.c`, lines 176-179

**Problem:** When `currentIndex >= length`, the engine transitions to `ENGINE_IDLE` and sets `stopCause = ENGINE_STOP_CAUSE_FINISHED`, but it does not call `engineExecuteCallbacks` with `ENGINE_EVENT_FINISHED` or `ENGINE_EVENT_STOPPED`. Callbacks registered for session completion events will never fire. The C++ version fires `onSessionComplete` and `onSessionStop` callbacks.

**Fix:** Call `engineExecuteCallbacks(self, &(EngineEvent){ENGINE_EVENT_FINISHED})` before returning when the session is complete.

### 7. `engineRegisterCallback` uses `void*` for event type (callback.h:18, engine.c:249)

**Location:** `src/core/callback.h`, line 18; `src/core/engine.c`, line 249

**Problem:** The event parameter is typed as `void*` rather than `EngineEvent*`, losing compile-time type safety. Callers can accidentally pass pointers of the wrong type, causing undefined behavior. The C++ version uses a type-safe `Signal` class with template `emit<Args...>`.

**Fix:** Change the parameter type to `EngineEvent*` in both the declaration and implementation, and update callers accordingly.

### 8. `engineKeyPress_Strict` doesn't emit `ENGINE_EVENT_STOPPED` when session completes (engine.c:177-179)

**Location:** `src/core/engine.c`, lines 174-192

**Problem:** When the session finishes in strict mode, the engine stops but no callbacks are executed to notify listeners that the session ended via the `ENGINE_EVENT_STOPPED` event.

**Fix:** Call `engineExecuteCallbacks` with `ENGINE_EVENT_STOPPED` alongside the finish event.

### 9. Hardcoded text in `engineStart` overflows with longer content (engine.c:113)

**Location:** `src/core/engine.c`, line 113

**Problem:** The placeholder text fits, but `strcpy` is used without bounds checking. When real session text is assigned later, this could overflow the 4096-byte buffer.

**Fix:** Replace `strcpy` with `strncpy` (or `snprintf`) and ensure the text buffer size is enforced at all assignment points.

### 10. `engineKeyPress_Flow` advances index even on incorrect keystrokes (engine.c:210)

**Location:** `src/core/engine.c`, line 210

**Problem:** In Flow mode, `currentIndex` is incremented unconditionally after both correct and incorrect keystrokes. However, `totalKeystrokes` is also incremented unconditionally (line 211). This means `totalKeystrokes` will always equal `currentIndex` in Flow mode, making accuracy calculation trivial but also meaning the backspace logic (which decrements `currentIndex`) will cause `totalKeystrokes` to diverge from `currentIndex`. The C++ version handles this more cleanly by tracking keystrokes independently of cursor position.

**Fix:** Ensure `totalKeystrokes` and `currentIndex` are tracked independently and consistently across both modes.

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

### Phase 1 — Core Fixes (blockers)
- Fix backspace logic bug (Bug #1)
- Fix totalKeystrokes inflation on backspace (Bug #2)
- Build system activation (Improvement #1)
- Add test suite (Improvement #2)

### Phase 2 — Correctness & State Machine
- Replace `clock()` with wall-clock timing (Bug #3)
- Fix accumulated time truncation (Bug #4)
- Fix sentinel value conflict (Bug #5)
- Emit missing callbacks (Bugs #6, #8, Improvement #4)
- Add missing states (Improvement #11)
- Change duration to milliseconds (Improvement #12)

### Phase 3 — Feature Parity with C++ Version
- Implement timeout handling (Improvement #6)
- Implement content provider system (Suggestion #2)
- Implement content chunking/formatters (Suggestion #3)
- Implement SQLite persistence (Suggestion #4)
- Implement type-safe signal system (Suggestion #5)
- Implement logger (Suggestion #7)
- Implement auto-save (Suggestion #8)

### Phase 4 — Polish & Quality
- Type safety for callback event parameter (Bug #7)
- `strcpy` → safe alternatives (Bug #9)
- Error hierarchy expansion (Suggestion #6)
- Test access macros (Suggestion #9)
- Incorrect keystrokes counter (Improvement #7)
- Header dependency cleanup (Improvement #8)
- NULL-check consistency (Improvement #9)
- Memory optimization (Suggestion #11)
- API documentation (Suggestion #12)
- Continuous integration (Suggestion #10)