# Changes v1.1.0 → v1.3.0

## Features
- Added `engineTick(Engine* self)` API for manual timer updates and timeout checks, useful for game loops.

## Bug fixes

### #8 — `incorrectKeystrokes` not decremented on backspace in Flow mode
Fixed: both `totalKeystrokes` and `incorrectKeystrokes`/`correctKeystrokes` are now decremented on backspace (net-effective accuracy). Backspace + retype correct → 100% accuracy.

### #9 — `signalInit` never called
Fixed: all 11 `Signal` instances are now explicitly initialized via `signalInit()` in `engineCreate`.

## Tests updated
- `tests/test_engine.c` — 3 backspace tests updated for net-effective accuracy; 1 new test for `engineTick`.

## Documentation
- `README.md` — Updated with `engineTick` API, EngineSnapshot API, commit convention, version macros, correct `loggerLogToStdout` name, updated test counts, and usage example.

## Diff (core only, excl. docs/CI infra)

```diff
diff --git a/src/core/engine.c b/src/core/engine.c
index aee0c4d..d127b13 100644
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
@@ -456,6 +470,12 @@ void engineBackspacePress(Engine *self) {
     } else if (self->mode == FlowMode) {
         engineBackspacePress_Flow(self);
     }
 }
 
+void engineTick(Engine* self) {
+    if (!self || self->state != ENGINE_RUNNING) return;
+    updateTime(self->session);
+    checkTimeout(self);
+}
+
diff --git a/src/core/engine.h b/src/core/engine.h
index b71d806..1641be4 100644
--- a/src/core/engine.h
+++ b/src/core/engine.h
@@ -164,6 +164,11 @@ void engineKeyPress(Engine* self, char key);
 /// @param self The Engine instance.
 void engineBackspacePress(Engine* self);
 
+/// @brief Manually update the engine's internal session timer and check for timeout.
+///        Useful for driving the engine in game loops without keystrokes.
+/// @param self The Engine instance.
+void engineTick(Engine* self);
+
 #ifdef __cplusplus
 }
 #endif
diff --git a/tests/test_engine.c b/tests/test_engine.c
index 1641be4..e9f3b21 100644
--- a/tests/test_engine.c
+++ b/tests/test_engine.c
@@ -959,6 +959,27 @@ static void test_timeout_pause_does_not_accumulate(void) {
     PASS();
 }
 
+static void test_engine_tick(void) {
+    TEST("Engine: engineTick manually advances time");
+    Engine* e = createTestEngine(StrictMode, 1);
+    ASSERT(e != NULL, "engineCreate returned NULL");
+    
+    engineStart(e);
+    ASSERT(engineIsRunning(e), "should be running");
+    
+#ifdef _WIN32
+    Sleep(1100);
+#else
+    struct timespec ts = {1, 100000000L};
+    nanosleep(&ts, NULL);
+#endif
+
+    engineTick(e);
+    ASSERT(engineIsTimedOut(e), "should be timed out after engineTick");
+    
+    engineDestroy(e);
+    PASS();
+}
+
 static void test_timeout_backspace_checks_timeout(void) {
     TEST("Timeout: backspace also triggers timeout check");
     Engine* e = createTestEngine(StrictMode, 1);
@@ -1368,6 +1389,7 @@ int main(void) {
     test_timeout_triggers();
     test_timeout_zero_disabled();
     test_timeout_pause_does_not_accumulate();
+    test_engine_tick();
     test_timeout_backspace_checks_timeout();
     test_auto_save_session();
```
