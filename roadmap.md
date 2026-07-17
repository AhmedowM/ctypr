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
| Strict & Flow modes | ✅ Complete | ✅ Implemented | |
| WPM / accuracy stats | ✅ Complete | ⚠️ Partial | Duration in seconds (C++ uses ms), no incorrect counter |
| Timeout handling | ✅ Complete | ❌ Missing | |
| Content chunking | ✅ Complete | ✅ Implemented | |
| String content provider | ✅ Complete | ✅ Implemented | |
| File content provider | ✅ Complete | ✅ Implemented | |
| Database content provider | ✅ Complete | ✅ Implemented | |
| SQLite persistence | ✅ Complete | ✅ Implemented | |
| Exception hierarchy | ✅ Complete | ❌ Missing | |
| Signal/callback system | ✅ Complete | ⚠️ Partial | Per-event callbacks with EngineEvent* parameter, no disconnect |
| Logger | ✅ Complete | ✅ Implemented | |
| Session data struct | ✅ Complete | ✅ Implemented | |
| Auto-save | ✅ Complete | ❌ Missing | |
| CMake build system | ✅ Complete | ✅ Implemented | |
| CMake install/packaging | ✅ Complete | ❌ Missing | |
| Test suite (72 tests) | ✅ Complete | ✅ Implemented | |
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

### 1. Build system activation ✅ Completed

### 2. Missing test suite ✅ Completed

### 3. Empty feature directories ✅ Completed

### 4. `ENGINE_EVENT_STARTED` callback never fired ✅ Completed

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

### 8. Missing `#include "event.h"` in callback.h ✅ Completed

### 9. NULL-pointer safety inconsistencies

**Location:** Various functions in `engine.c`

**Problem:** Some functions check for NULL `self`, some don't. For example, `engineSetMode` checks for NULL but `engineDestroy` also checks. The pattern is inconsistent.

**Improvement:** Standardize NULL checking — either validate in all functions or accept crashes on NULL as a precondition violation with a clear contract.

### 10. Use of `strcpy`, `sprintf`, and raw buffers

**Improvement:** Replace `strcpy` with `snprintf` or `strncpy` throughout. Consider using dynamic string allocation or safer buffer management utilities.

### 11. State machine missing states ✅ Completed (design decision — see note below)

**Note:** The C version uses `EngineStopCause` as secondary metadata alongside `EngineState` instead of baking stop reasons into the state enum. This is a **better approach for C** — it avoids state explosion and separates "what is the engine doing?" from "why did it stop?".

### 12. Duration should be in milliseconds ✅ Completed

---

## Suggestions

### 1. Implement wall-clock timing with sub-second precision ✅ Completed

### 2. Implement content provider system ✅ Completed

### 3. Implement content chunking / formatters ✅ Completed

### 4. Implement SQLite persistence ✅ Completed

### 5. Implement type-safe callback/signal system

Port the C++ version's Signal class:
- Per-event callback lists (one list per event type) instead of iterating all callbacks
- `connect(Slot)` with disconnect token support
- `connectSimple(Slot)` without disconnect
- `emit(Args...)` to fire all connected slots
- `disconnect(token)` to remove specific connections
- `compact()` to clean up expired token slots
- `clear()` to remove all connections

### 6. Implement exception/error hierarchy

Port the C++ version's exception hierarchy to C error codes:
- `ENGINE_ERROR_NONE` — No error
- `ENGINE_ERROR_CONFIG` — Configuration error (invalid mode, unknown option)
- `ENGINE_ERROR_STATE` — State machine error (invalid transition)
- `ENGINE_ERROR_PROVIDER` — Provider error (database query failure)
- `ENGINE_ERROR_CONTENT` — Content error (empty file, invalid format)
- `ENGINE_ERROR_FILE` — File I/O error (file not found, permission denied)

### 7. Implement logger ✅ Completed

### 8. Add auto-save functionality

Port the C++ version's `setAutoSave(bool)`:
- When enabled, automatically persist session data on completion/timeout
- Integrate with the Repository module

### 9. Implement test access macros

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
- Backspace logic inversion
- `totalKeystrokes` inflation on backspace
- `clock()` replaced with wall-clock timing
- Accumulated time truncation
- Sentinel value conflict
- Missing finish event
- Type-unsafe `void*` in callbacks
- Missing stopped event
- `strcpy` overflow risk
- Flow mode keystroke tracking
- `ENGINE_EVENT_STARTED` callback
- Header dependency cleanup
- Duration changed to milliseconds

### ✅ Phase 2 — Build & Test Infrastructure (completed)
- Build system activation
- Add test suite
- Empty feature directories populated

### ✅ Phase 3 — Feature Parity (partially completed)
- Implement content provider system ✅
- Implement content chunking/formatters ✅
- Implement SQLite persistence ✅
- Implement logger ✅
- Implement timeout handling ❌
- Implement type-safe signal system ❌
- Implement auto-save ❌

### Phase 4 — Polish & Quality
- Error hierarchy expansion
- Test access macros
- Incorrect keystrokes counter
- NULL-check consistency
- Memory optimization
- API documentation
- Continuous integration
- Engine struct visibility
