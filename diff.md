# Changes v1.3.0 → v1.5.1

## v1.4.0 — Pipeline alignment + content filter API

### Features
- **New content filter API** (`content.h`):
  - `contentProviderSetDifficultyFilter(cp, "Easy"|"Normal"|"Hard"|"Expert"|NULL)` — filters sentences by difficulty. NULL clears the filter.
  - `contentProviderSetWordLengthRange(cp, min_len, max_len)` — filters words by length range. Pass 0 for both to clear.
  - Both filters dynamically build WHERE clause parameters; persist across `reset()`.

### DB schema changes
- `words.db` consolidated: `common_words` + `random_words` tables replaced by a single `words` table with a `frequency_rank` column.
- `CONTENT_MODE_COMMON_WORDS` now queries `ORDER BY frequency_rank ASC` (most frequent first).
- `CONTENT_MODE_RANDOM_WORDS` now queries `ORDER BY RANDOM()`.
- `sentences.db` unchanged (10,400 rows, 192K word tokens).

### Bug fixes
- DB content provider queries now target the correct table names (`words` instead of `common_words`/`random_words`).

### Pipeline (Python scripts in `src/db/scripts/`)
- New `run_pipeline.py` — orchestrates all stages: fetch → clean → analyze → score → build_db.
- Fetch sources: DummyJSON (1,454 quotes), Wikipedia (11K sents, BATCH_SIZE=20, 2.4× faster, zero 429s), NPR/NYT news.
- Word frequency lists: 120K entries from multiple sources, deduplicated, capped at 120K.
- Robust error handling: retry with exponential backoff, timeout hardening (300→600s), `getattr` vs `dict.get` bug fix.
- Score tuning: 40-70 WPM range cap, length bonus above 30 chars, 0.2 floor.

### Doxygen cleanup
- `@param` / `@return` annotations added to all public headers; stale comments removed.

---

## v1.5.0 — Thread-safe Repository + `getSessionsByMode`

### API additions
- `SessionData* repositoryGetSessionsByMode(Repository* repo, const char* mode, size_t* count)` — filters sessions by mode ("strict" or "flow") in SQL, avoiding a manual post-filter loop.

### Thread-safety contract (documented in header comments)
- **Engine**: NOT thread-safe (caller must serialize access to a single `Engine*`)
- **ContentProvider**: NOT thread-safe (caller must serialize access to a single `ContentProvider*`)
- **Repository**: IS thread-safe (all public functions are mutex-protected)
- **Logger**: IS thread-safe (all public functions are mutex-protected)

### Internal changes
- Repository struct now has a `ctypr_mutex_t lock` — wraps all 14 public functions with `MUTEX_LOCK`/`MUTEX_UNLOCK`.
- `repositoryGetBestWpm()` / `repositoryGetBestRawWpm()` refactored into shared `getBestByColumn()` internal helper.

### Default behavior
- `repositoryCreate(NULL)` defaults database path to `"typr.db"` (unchanged, now documented).

---

## v1.5.1 — DRY refactor + logger thread-safety fix + concurrency tests

### API changes
- None (all changes are internal or test-only).

### Thread-safety fix (Logger)
- `loggerSetLevel()` — previously wrote `currentLevel` without the mutex. Now acquires the lock.
- `loggerGetLevel()` — previously read `currentLevel` without the mutex. Now acquires the lock.
- `loggerLogToStdout()` — previously wrote `stdoutEnabled` without the mutex. Now acquires the lock.
- `loggerLog()` — previously checked `level < currentLevel` outside the lock. Now checks inside the lock.

### Internal refactors
- **`src/core/platform_internal.h`** (new): consolidates `ctypr_mutex_t`/`MUTEX_*` macros, `ctypr_thread_t`/`THREAD_*` macros, and `strdup` platform define — replaces 16-line duplicated blocks in `repository.c` and `logger.c`.
- **`src/core/engine.c`**:
  - Extracted `signalForEvent(Engine*, EngineEvent)` helper — kills a 12-line `switch` duplicated in `engineDisconnect()` and `engineClearEvent()`.
  - `engineGetSnapshot()` now calls `engineGetStats(engine)` instead of re-implementing stat population inline.

### Test changes
- **`tests/test_common.h`** (new): shared test framework macros (`TEST`/`PASS`/`FAIL`/`ASSERT`), `TEST_SUMMARY()`, `SLEEP_MS()`, `TEST_DB_HELPERS()`. Replaces ~27 lines duplicated across all 6 test files.
- **`tests/test_concurrency.c`** (new): 5 tests for thread safety:
  1. Repository: 4 threads × 50 concurrent saves → verify count = 200 + data integrity
  2. Repository: 2 writers + 2 readers running concurrently
  3. Logger: 4 threads × 100 concurrent log messages to a shared file
  4. Logger: 3 log threads + 1 reconfigure thread (toggling level/stdout)
  5. NULL safety for all concurrency-relevant APIs
- **`tests/CMakeLists.txt`**: new `ctypr_add_test()` function eliminates 8-line target boilerplate.
- 4 platform-specific `Sleep`/`nanosleep` blocks in `test_engine.c` replaced with `SLEEP_MS()`.

### Test counts
- Engine: 51 tests (unchanged)
- Content: 19 tests (unchanged)
- Formatter: 8 tests (unchanged)
- Repository: 12 tests (unchanged)
- Logger: 15 tests (unchanged)
- Concurrency: 5 tests (new)
- **Total: 98 tests** (was 93 in v1.5.0, 91 in v1.4.0, ~80-90 in v1.3.0)

---

## Summary: API surface comparison

| API | v1.3.0 | v1.5.1 |
|-----|--------|--------|
| `contentProviderSetDifficultyFilter` | ❌ | ✅ |
| `contentProviderSetWordLengthRange` | ❌ | ✅ |
| `repositoryGetSessionsByMode` | ❌ | ✅ |
| `engineTick` | ✅ (v1.2.0) | ✅ |
| Thread-safety docs | ❌ | ✅ (all public headers) |
| Mutex on Repository | ❌ | ✅ |
| Logger full mutex coverage | ❌ (3 functions missing) | ✅ |
| Shared internal headers | ❌ | `platform_internal.h` |
| Shared test headers | ❌ | `test_common.h` |
| Concurrency tests | ❌ | 5 tests |
| DB pipeline scripts | ❌ | 9 Python scripts |
