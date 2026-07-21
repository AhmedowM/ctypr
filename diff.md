# Changes since v1.0.0

Version: **v1.1.0**
Files changed: 6 (5 modified, 1 new)
Lines: +361 / -10

---

## New public API

### `src/core/snapshot.h` (new file)
- `EngineSnapshot` struct — a point-in-time copy of everything a UI needs to render a typing session:
  - `text[4096]`, `length`, `cursorIndex`
  - `expectedChar` — character the user should type next (or `'\0'` at end of text)
  - `incorrectFlags[4096]` — per-character bool array; `true` where a wrong key was recorded
  - `stats` — embedded `SessionStats` (wpm, accuracy, duration, keystroke counts)
  - `state` + `stopCause` — current engine operational state
- `engineGetSnapshot(Engine*)` — returns a fully-populated `EngineSnapshot` by value; snapshot is an independent copy (mutating it does not affect the engine); NULL engine returns `ENGINE_ERROR` / `ENGINE_STOP_CAUSE_ERROR`.

### `src/core/engine.c`
- Implemented `engineGetSnapshot()` (48 lines).
- Added `#include "snapshot.h"`.

### `src/core/ctypr.h`
- Added `#include "ctypr/snapshot.h"` so the umbrella header exposes `EngineSnapshot` automatically.

---

## Bug fixes

### `src/db/repository.c`
- **Tilde expansion path separator** (`getFullPath`): `snprintf(full, …, "%s%s", home, dbPath+1)` → `"%s/%s"`. Without the `/`, `~/.config/typr.db` was concatenated as `/home/usertypo.db` instead of `/home/user/.config/typr.db`.
- Also corrected the length calculation comment: `strlen(home) + strlen(dbPath) + 1` (was missing `+1` in source, only present in comment).

### `src/core/engine.c`
- **POSIX `timeDiffMs` borrow bug**: When `end->tv_nsec < start->tv_nsec`, `nsecDiff` is negative; `secDiff` was not decremented, producing a time delta that was ~1 s too large. Fixed with the standard borrow pattern:
  ```c
  if (nsecDiff < 0) { secDiff--; nsecDiff += 1000000000; }
  ```
- **Thread-unsafe `localtime`**: Replaced `localtime(&now)` (returns pointer to static buffer) with `localtime_r` on POSIX and `localtime_s` on Windows.
- **Timeout multiplication overflow risk**: Changed `(int64_t)self->timeout * 1000` → `* 1000LL`. While `uint16_t * 1000` always fits in a 32-bit `int`, the cast-to-`int64_t` was applied before the multiplication, which is non-portable on 16-bit targets.
- **`onSegmentCompleted` signal now fires on session finish**: The signal slot existed but was never emitted. It now fires exactly once in `completeSession()` — immediately before `onFinished` and `onStopped` — so UI code can detect full-text completion in a single callback.

### `src/content/content.c`
- **Uninitialized `n` in `_cpGetFile`**: `size_t n;` declared outside the `while` loop body. If the loop never executes (empty file or first `fread` returns 0), `n` was uninitialized when tested after the loop. Changed to `size_t n = 0` and replaced the post-loop `n > 0` check with a `truncated` boolean flag set inside the loop.
- **Missing `default` in DB mode switch**: `switch (cp->mode)` in `_cpGetDatabase` had no `default`, leaving `query` as NULL on an unknown mode, which would crash `sqlite3_prepare_v2`. Added `default` that logs a warning and falls back to `typing_sentences`.
- **`sqlite3_bind_int64` return unchecked**: Now checks `rc != SQLITE_OK`, logs an error, cleans up the prepared statement and database handle, and returns an empty chunk instead of silently proceeding with an unbound parameter.

---

## New tests

### `tests/test_engine.c` — 10 new tests (+241 lines)

**EngineSnapshot tests (8):**
| Test | What it verifies |
|------|-----------------|
| `test_snapshot_null_engine` | NULL → `ENGINE_ERROR` / `ENGINE_STOP_CAUSE_ERROR` / zeroed fields |
| `test_snapshot_before_start` | Idle engine: empty text, `cursorIndex == 0`, `expectedChar == '\0'` |
| `test_snapshot_after_start` | Running engine: text populated, `cursorIndex == 0`, `expectedChar == 'T'` |
| `test_snapshot_progress_and_flags` | Strict mode: cursor advances correctly, `incorrectFlags[1] == true` |
| `test_snapshot_flow_mode_incorrect_flags` | Flow mode: wrong key advances cursor, flag set at wrong position |
| `test_snapshot_after_backspace` | Backspace clears `incorrectFlags[i]` and restores cursor |
| `test_snapshot_after_completion` | Completed session: `cursorIndex == length`, `expectedChar == '\0'` |
| `test_completion_copies_text` | Mutating snapshot text does not affect engine internals |

**SEGMENT_COMPLETED signal tests (2):**
| Test | What it verifies |
|------|-----------------|
| `test_segment_completed_fires_on_finish` | Callback fires exactly once when text is fully typed |
| `test_segment_completed_not_on_user_stop` | Callback does NOT fire on `engineStop()` |

**NULL safety:** `engineGetSnapshot(NULL)` and `engineOnSegmentCompleted(NULL,…)` added to the existing NULL safety test.

---

## Summary of behavioural changes

| Change | Backwards compatible? |
|--------|----------------------|
| `~` path now expands with `/` separator | Yes (fixes incorrect path) |
| `timeDiffMs` borrow fixed | Yes (time values were *slightly* wrong before) |
| `localtime` → `localtime_r`/`localtime_s` | Yes (same result, thread-safe) |
| `onSegmentCompleted` now fires on finish | Yes (was dead code before — nobody could have been listening) |
| `sqlite3_bind_int64` failure returns empty chunk | Yes (prevents crash on DB failure) |
| `EngineSnapshot` struct + `engineGetSnapshot` | Yes (additive, new symbol) |
