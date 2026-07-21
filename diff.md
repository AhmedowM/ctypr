# Changes v1.1.0 → v1.2.0

3 files changed, 93 insertions(+), 46 deletions(-) (excluding docs/CI infra: +7 files, +175 lines)

## Bug fixes

### #8 — `incorrectKeystrokes` not decremented on backspace in Flow mode

`src/core/engine.c:425-440`: `engineBackspacePress_Flow` now decrements both `totalKeystrokes` and the per-type counter (`incorrectKeystrokes` / `correctKeystrokes`) on backspace. Net-effective accuracy: backspace + retype correct → 100% accuracy.

### #9 — `signalInit` never called

`src/core/engine.c:215-225`: All 11 `Signal` instances are now explicitly initialized via `signalInit()` in `engineCreate`.

## Tests updated

`tests/test_engine.c` — 3 tests updated for net-effective accuracy:

| Test | Old assertion | New assertion |
|------|-------------|-------------|
| `test_flow_backspace_correct` | `totalKeystrokes == 2` | `== 1` |
| `test_flow_backspace_incorrect` | `totalKeystrokes == 1`, checked `correctKeystrokes` | `== 0`, checks `incorrectKeystrokes` |
| `test_flow_backspace_retry` | `totalKeystrokes == 2`, `accuracy == 50.0` | `== 1`, `accuracy == 100.0` |

## Documentation

`README.md` — Rewritten Engine/Stats/Signals/Logger API reference; adds Snapshot API, version macros, commit convention + setup scripts; corrects `loggerLogToStdout` name; updates test count to 100+; removes stale "What's left" section.

## Diff (core only, excl. docs/CI infra)

```diff
diff --git a/src/core/engine.c b/src/core/engine.c
index aee0c4d..04bcd1d 100644
--- a/src/core/engine.c
+++ b/src/core/engine.c
@@ -212,6 +212,18 @@ Engine* engineCreate(const EngineConfig* config) {
     engine->contentProvider = config->contentProvider;
     engine->autoSaveRepo = config->autoSaveRepo;
     engine->autoSaveEnabled = config->autoSaveEnabled;
+    signalInit(&engine->onStarted);
+    signalInit(&engine->onStopped);
+    signalInit(&engine->onPaused);
+    signalInit(&engine->onResumed);
+    signalInit(&engine->onTimeout);
+    signalInit(&engine->onFinished);
+    signalInit(&engine->onCorrectKeystroke);
+    signalInit(&engine->onIncorrectKeystroke);
+    signalInit(&engine->onBackspace);
+    signalInit(&engine->onSegmentCompleted);
+    signalInit(&engine->onError);
+
     engine->session = calloc(1, sizeof(Session));
     if (!engine->session) {
         fprintf(stderr, "[ERROR] Failed to allocate Engine session\n");
@@ -428,8 +440,10 @@ void engineBackspacePress_Flow(Engine *self) {
     if (self->session->currentIndex <= 0) return;
     self->session->currentIndex--;
     uint8_t *was_incorrect = &self->session->incorrectKeystrokesBitmap[self->session->currentIndex];
+    self->stats.totalKeystrokes--;
     if (*was_incorrect == 1) {
         *was_incorrect = 0;
+        self->stats.incorrectKeystrokes--;
         if (self->logger) loggerLog(self->logger, LOG_LEVEL_DEBUG, "Flow: backspace over incorrect key");
     } else {
         self->stats.correctKeystrokes--;
diff --git a/tests/test_engine.c b/tests/test_engine.c
index b71d806..1641be4 100644
--- a/tests/test_engine.c
+++ b/tests/test_engine.c
@@ -349,7 +349,7 @@ static void test_flow_backspace_correct(void) {
     engineBackspacePress(e);
     
     stats = engineGetStats(e);
-    ASSERT(stats.totalKeystrokes == 2, "totalKeystrokes should still be 2");
+    ASSERT(stats.totalKeystrokes == 1, "totalKeystrokes should be 1 after undoing a keystroke");
     ASSERT(stats.correctKeystrokes == 1, "correctKeystrokes should be 1 after undoing correct char");
     
     engineDestroy(e);
@@ -373,12 +373,12 @@ static void test_flow_backspace_incorrect(void) {
     engineBackspacePress(e);
     
     stats = engineGetStats(e);
-    ASSERT(stats.totalKeystrokes == 1, "totalKeystrokes should still be 1");
-    ASSERT(stats.correctKeystrokes == 0, "correctKeystrokes should still be 0");
+    ASSERT(stats.totalKeystrokes == 0, "totalKeystrokes should be 0 after undoing the only keystroke");
+    ASSERT(stats.incorrectKeystrokes == 0, "incorrectKeystrokes should be 0 after undoing incorrect key");
     
     engineKeyPress(e, 'T');
     stats = engineGetStats(e);
-    ASSERT(stats.totalKeystrokes == 2, "totalKeystrokes should be 2");
+    ASSERT(stats.totalKeystrokes == 1, "totalKeystrokes should be 1");
     ASSERT(stats.correctKeystrokes == 1, "correctKeystrokes should be 1");
     
     engineDestroy(e);
@@ -865,14 +865,14 @@ static void test_flow_backspace_retry(void) {
     
     engineBackspacePress(e);
     stats = engineGetStats(e);
-    ASSERT(stats.totalKeystrokes == 1, "totalKeystrokes should still be 1");
-    ASSERT(stats.correctKeystrokes == 0, "correctKeystrokes should still be 0");
+    ASSERT(stats.totalKeystrokes == 0, "totalKeystrokes should be 0 after undoing the only keystroke");
+    ASSERT(stats.incorrectKeystrokes == 0, "incorrectKeystrokes should be 0 after undoing incorrect key");
     
     engineKeyPress(e, 'T');
     stats = engineGetStats(e);
-    ASSERT(stats.totalKeystrokes == 2, "totalKeystrokes should be 2");
+    ASSERT(stats.totalKeystrokes == 1, "totalKeystrokes should be 1");
     ASSERT(stats.correctKeystrokes == 1, "correctKeystrokes should be 1");
-    ASSERT(stats.accuracy == 50.0, "accuracy should be 50.0");
+    ASSERT(stats.accuracy == 100.0, "accuracy should be 100.0 after correction");
     
     engineDestroy(e);
     PASS();
```
