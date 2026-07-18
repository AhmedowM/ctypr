# ctypr Project Roadmap

## Project Overview

ctypr is a **C rewrite** of the C++ typing practice library [typr-cpp](https://github.com/your-username/typr-cpp). The C++ version is fully featured with content providers, SQLite persistence, chunking, logging, 62 test cases, and a complete CMake build system. This C port is in early stages — core engine logic exists but most features are not yet implemented.

**Reference C++ project location:** `C:\Users\Secondary\Projects\typr-cpp`

---

## C++ Reference Feature Map

The table below shows which features from the C++ version exist in the C port, which are partially implemented, and which are missing entirely.

| Feature | C++ Status | C Status | Notes |
|---------|-----------|----------|-------|
| Session state machine | ✅ Complete | ⚠️ Partial | Missing `ChunkFinished` state |
| Strict & Flow modes | ✅ Complete | ✅ Implemented | |
| WPM / accuracy stats | ✅ Complete | ✅ Implemented | Includes explicit incorrectKeystrokes counter |
| Timeout handling | ✅ Complete | ✅ Implemented | |
| Content chunking | ✅ Complete | ✅ Implemented | |
| String content provider | ✅ Complete | ✅ Implemented | |
| File content provider | ✅ Complete | ✅ Implemented | |
| Database content provider | ✅ Complete | ✅ Implemented | |
| SQLite persistence | ✅ Complete | ✅ Implemented | |
| Exception hierarchy | ✅ Complete | ❌ Missing | |
| Signal/callback system | ✅ Complete | ✅ Implemented | Per-event registration functions, disconnect, clear |
| Logger | ✅ Complete | ✅ Implemented | |
| Session data struct | ✅ Complete | ✅ Implemented | |
| Auto-save | ✅ Complete | ✅ Implemented | |
| CMake build system | ✅ Complete | ✅ Implemented | |
| CMake install/packaging | ✅ Complete | ❌ Missing | |
| Test suite (90 tests) | ✅ Complete | ✅ Implemented | |
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

### 6. Missing timeout implementation ✅ Completed

**Location:** `src/core/engine.c`

**Problem:** The engine has a `timeout` field that can be set and retrieved, but the timeout was never checked during execution. Sessions would never automatically stop due to timeout.

**Solution:** Added `checkTimeout()` called at the start of `engineKeyPress_Strict`, `engineKeyPress_Flow`, and `engineBackspacePress_Flow`. Emits `ENGINE_EVENT_TIMEOUT` and transitions to `ENGINE_STOP_CAUSE_TIMEOUT`. Paused time is excluded from accumulation.

### 7. No incorrect keystrokes counter in SessionStats

**Location:** `src/core/stats.h`

**Problem:** `SessionStats` has `correctKeystrokes` and `totalKeystrokes` but no explicit `incorrectKeystrokes` counter. Accuracy is calculated from these two values. With the backspace bugs, this can produce accuracy > 100% or other corrupt values. The C++ version's `Stats` struct also derives incorrect from total - correct, but handles it more carefully.

**Improvement:** Add an explicit `incorrectKeystrokes` field to `SessionStats` to simplify calculations and provide a single source of truth independent of potential counter corruption.

### 8. Missing `#include "event.h"` in callback.h ✅ Completed

### 9. NULL-pointer safety inconsistencies ⚠️ Partial progress

**Location:** Various functions in `engine.c`

**Problem:** Some functions check for NULL `self`, some don't. The pattern is inconsistent.

**Improvement:** Signal registration functions added in the refactor all check for NULL `engine`. The lifecycle and state query functions already check. A full audit of all 50+ functions is still pending.

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

### 5. Implement type-safe callback/signal system ✅ Completed

Each event type owns an opaque `Signal` embedded in the `Engine` struct. Per-event registration functions (`engineOnStarted`, etc.) connect callbacks. Supports disconnect by slot index, clear per event, and 5 simultaneous slots per event type.

### 6. Implement exception/error hierarchy ✅ Completed

Port the C++ version's exception hierarchy to C error codes:
- `ENGINE_ERROR_NONE` — No error
- `ENGINE_ERROR_CONFIG` — Configuration error (invalid mode, unknown option)
- `ENGINE_ERROR_STATE` — State machine error (invalid transition)
- `ENGINE_ERROR_PROVIDER` — Provider error (database query failure)
- `ENGINE_ERROR_CONTENT` — Content error (empty file, invalid format)
- `ENGINE_ERROR_FILE` — File I/O error (file not found, permission denied)

### 7. Implement logger ✅ Completed

### 8. Add auto-save functionality ✅ Completed

Port the C++ version's `setAutoSave(bool)`:
- When enabled, automatically persist session data on completion/timeout
- Integrate with the Repository module

### 9. Implement test access macros

Port the C++ version's `TYPR_ENABLE_TEST_ACCESS` pattern:
- Conditionally expose internal state for testing
- `getContent()`, `getIndex()`, `getChunkCount()`, `getChunkIndex()`
- `setFormatter()` for testing different formatters

### 10. Add continuous integration ✅ Completed

Set up GitHub Actions (or similar) to:
- Build on Windows, macOS, and Linux
- Run the test suite on every push/PR
- Check code formatting with clang-format
- Run static analysis (cppcheck, clang-tidy)

### 11. Optimize memory usage ✅ Completed (partial)

The `incorrectKeystrokesIndices` array was changed from `uint16_t[4096]` = 8 KB to `uint8_t[4096]` = 4 KB (50% reduction). Full bitset (512 bytes) not yet implemented.

### 12. Create proper API documentation

Add comprehensive Doxygen-style documentation matching the C++ version:
- Document all public functions with `@param`, `@return`, `@see` tags
- Provide usage examples in header comments
- Generate HTML documentation as part of the build

---

---

## Micro-Bugs

Identified during code review (July 2026):

| # | File | Issue | Severity | Status |
|---|------|-------|----------|--------|
| 1 | `engine.c:252-267` | `engineStop` does not call `autoSaveSession`. User-initiated stop silently discards session data. C++ reference auto-saves on all stop causes. | Low | ✅ Fixed |
| 2 | `engine.c:252-258` | Calling `engineStop()` on a finished/timed-out engine prints "engineStop: not running" and sets `lastError = NOT_RUNNING`, which is misleading since the engine did run previously. | Very Low | ❌ |
| 3 | `content.c:165-187` | `_cpGetFile` opens file in text mode (`"r"` not `"rb"`). On Windows this translates CRLF → LF, making byte counts differ from actual file size. | Very Low | ✅ Fixed |
| 4 | `content.c:173` | File larger than 4095 bytes is silently truncated. No warning to caller. | Low | ✅ Fixed |
| 5 | `repository.c:33-38` | `getFullPath` has dead code: checks for `"~/.typr/typr.db"` but never expands `~`. The fallback to `"typr.db"` always activates. | Very Low | ✅ Fixed |
| 6 | `formatter.c:64-72` | Sentence-boundary search picks the *first* sentence end after midpoint. With two potential boundaries close together, produces unexpectedly short chunks. Matches C++ behavior, but the result can vary by position. | Minimal | ❌
| 7 | `test_engine.c` (auto-save test) | `sessions[0].accuracy == 100.0` uses double `==`. FP rounding can fail on different platforms. Should use `fabs(acc - 100.0) < 0.001`. | Low | ✅ Fixed |
| 8 | `engine.c:151-166` | `checkTimeout` dereferences `self->session` without NULL check. Safe for valid engines (session always allocated), but any future code path that NULLs session would cause a crash. | Very Low | ✅ Fixed |
| 9 | `engine.c:300-306` | No `engineWasStopped()` to query the *last* stop cause independently of current state. `engineGetStateInfo` reveals it, but there's no dedicated predicate. | Minimal | ✅ Fixed |

---

## Micro-Optimizations

Identified during code review (July 2026):

| # | File | Current | Optimized | Saving | Status |
|---|------|---------|-----------|--------|--------|
| 1 | `Session.incorrectKeystrokesIndices` | `uint16_t[4096]` = 8 KB per session (boolean values only) | `uint8_t[4096]` = 4 KB | 50% | ✅ Done |
| 2 | `clearArray()` in `engine.c` | Manual `for` loop zeroing each element | `memset(array, 0, size * sizeof(type))` | ~10× faster | ✅ Done |
| 3 | `timeDiffMs` freq init on Windows | `static LARGE_INTEGER freq` + branch-per-call | Pre-init in `engineCreate` or global once | Negligible | ❌ |
| 4 | `repository.c` DB handle pattern | Open → prepare → step → finalize → close on every CRUD call | Keep `sqlite3*` open in `Repository` struct; close only on `repositoryDestroy` | ~1 ms per query | ✅ Done |
| 5 | `engineGetStats` timestamp | Calls `localtime` + `strftime` on every stats poll | Cache timestamp; regenerate only on session boundaries | ~5-10 µs per call | ✅ Done |
| 6 | `snprintf("%s", src)` pattern | Used for all string copies throughout codebase | `memcpy(dst, src, len + 1)` in non-hot paths | Format-string parsing overhead | ❌ |
| 7 | `struct Session` field ordering | `size_t` + `uint32_t` + `uint16_t[4096]` creates inter-field padding | Group same-size fields; put large array first | ~6 bytes waste (negligible) | ❌ |
| 8 | `engineKeyPress_Strict` vs `_Flow` | 30+ lines of near-identical code in two functions | Single function with `bool advanceAlways` parameter | Code size, maintainability | ✅ Done |
| 9 | `autoSaveSession` WPM recomputation | Recomputes accuracy/WPM inline (duplicates `engineGetStats`) | Extract shared helper function | Code size, 0 runtime (one call per session end) | ✅ Done |

---

## Phase 6 — Architecture Refactoring ✅ Completed

After reviewing the overall project structure, the following architectural changes were decided:

### A. Logger Propagation (engine → children)

**Decision:** Keep per-component `Logger*` fields and `*SetLogger()` functions for standalone use. Add auto-propagation when components are wired through the engine.

**Changes:**
- `engineSetLogger(engine, log)` internally calls:
  - `contentProviderSetLogger(engine->contentProvider, log)` if provider is set
  - `repositorySetLogger(engine->autoSaveRepo, log)` if repo is set
- File/Web providers internally propagate logger to their internal formatter
- No global singleton — user creates one Logger, sets it on the engine, engine handles distribution

### B. EngineConfig Struct

**Changes:**
- Replace `engineCreate(EngineMode, uint16_t)` with `engineCreate(const EngineConfig*)`
- Config struct:
```c
typedef struct EngineConfig {
    EngineMode mode;
    uint16_t timeout;
    ContentProvider* contentProvider;  // required — engineCreate fails if NULL
    Repository* autoSaveRepo;          // optional (NULL = no auto-save)
    bool autoSaveEnabled;
} EngineConfig;
```
- `engineCreate(NULL)` or `config.mode == UnknownMode` or `config.contentProvider == NULL` → returns NULL (fail)
- Keep setter overrides for runtime changes: `engineSetMode`, `engineSetTimeout`, `engineSetAutoSave`, `engineSetContentProvider`
- Engine does NOT own component lifetimes — caller creates and destroys all components

### C. Content Provider Refactor

**Decision:** Formatting is handled internally per provider type. DB provider does not use a formatter (pre-polished sentences). File/Web providers create and use a Formatter internally.

**Changes:**
- Rename `ContentDbMode` → `ContentMode`, promote to all-provider scope:
```c
typedef enum ContentMode {
    CONTENT_MODE_SENTENCES,     // DB: typing_sentences table; File: lines
    CONTENT_MODE_COMMON_WORDS,  // DB: common_words table; File: unsupported (returns empty)
    CONTENT_MODE_RANDOM_WORDS   // DB: random_words table; File: shuffle all words
} ContentMode;
```
- `contentProviderSetDbMode` → `contentProviderSetMode`
- DB provider queries map to the correct table based on mode
- File provider: SENTENCES = split lines, RANDOM_WORDS = shuffle all words in file
- File provider creates `Formatter` internally for SENTENCES mode to chunk large files
- String provider: unchanged (testing fallback)
- Web provider: unchanged (falls back to string, future work)

### D. Component Name Field

**Decision:** Each public struct gets a `const char* name` field set internally by factory functions.

**Names:**
| Struct | Factory | `name` value |
|--------|---------|-------------|
| `Engine` | `engineCreate` | `"Engine"` |
| `ContentProvider` (string) | `contentProviderFromString` | `"StringProvider"` |
| `ContentProvider` (file) | `contentProviderFromFile` | `"FileProvider"` |
| `ContentProvider` (DB) | `contentProviderFromDatabase` | `"DbProvider"` |
| `ContentProvider` (web) | `contentProviderFromWeb` | `"WebProvider"` |
| `Formatter` | `formatterCreate` | `"Formatter"` |
| `Repository` | `repositoryCreate` | `"Repository"` |

- All log calls within a component use `loggerLog(self->name, level, msg)`
- No change to the `loggerLog` signature — it still takes an explicit `Logger*`
- The component name adds context to diagnostic output

### E. Engine Uses ContentProvider for Text

**Changes:**
- `engineStart` calls `contentProviderGetNext(self->contentProvider)` to get session text
- Removes hardcoded `"The quick brown fox jumps over the lazy dog."` from `engine.c`
- If content provider is NULL or returns empty content, `engineStart` fails with `ENGINE_ERROR_CONTENT` (new error code)
- `EngineConfig.contentProvider` must be non-NULL or `engineCreate` returns NULL

### F. Error Hierarchy Expansion

**Changes:**
- Add `ENGINE_ERROR_CONTENT` to `error.h` — for missing/empty content provider
- Add `ENGINE_ERROR_CONFIG` — for invalid config passed to `engineCreate`

### G. Test Updates

- All `engineCreate(mode, timeout)` calls → `engineCreate(&(EngineConfig){.mode=..., .timeout=..., .contentProvider=...})`
- All `*SetLogger()` calls remain for standalone component tests
- Engine integration tests call `engineSetLogger()` once, logger propagates automatically to provider/repo

---

## Implementation Priority (Updated)

### ✅ Phase 1 — Core Bug Fixes (completed)

### ✅ Phase 2 — Build & Test Infrastructure (completed)

### ✅ Phase 3 — Feature Parity (completed)

### ✅ Phase 6 — Architecture Refactoring (completed)

### ✅ Phase 4 — Polish & Quality (completed)

### Phase 5 — Documentation & Publishing
- API documentation (⚠️ partially done — all headers have `@brief`/`@param`/`@return`)
- CMake install/packaging
- Doxygen generation in build

### Phase 7 — Type Safety & Signal Enhancements
- Type-safe disconnect tokens (`SignalToken` is currently just a slot index)
- Event data passing (typed payload with signal emission)

### Remaining Small Items
- **Micro-bug #2** — `engineStop()` on finished/timed-out engine misleading "not running" warning
- **Micro-bug #6** — Formatter sentence-boundary search picks first boundary after midpoint (matches C++)
- **Micro-opt #3** — Windows `timeDiffMs` freq init per-call (negligible)
- **Micro-opt #6** — Replace `snprintf("%s", ...)` with `memcpy` (low value, many files)
- **Micro-opt #7** — `struct Session` field reordering (~6 bytes padding)
- **Test access macros** — Port C++ `TYPR_ENABLE_TEST_ACCESS` pattern
- **NULL-check consistency** — Full audit of all 50+ functions
- **Engine struct visibility** — Consider exposing for stack allocation
- **Web content provider** — Implement actual HTTP fetching instead of fallback string
