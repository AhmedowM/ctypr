#include "engine.h"
#include "callback.h"
#include "error.h"
#include "event.h"
#include "state.h"
#include "stats.h"
#include "repository.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#endif

static int tests_passed = 0;
static int tests_failed = 0;
static int test_count = 0;

#define TEST(name) do { \
    test_count++; \
    printf("  TEST %d: %s ... ", test_count, name); \
    fflush(stdout); \
} while(0)

#define PASS() do { \
    tests_passed++; \
    printf("PASSED\n"); \
} while(0)

#define FAIL(msg) do { \
    tests_failed++; \
    printf("FAILED: %s (line %d)\n", msg, __LINE__); \
} while(0)

#define ASSERT(cond, msg) do { \
    if (!(cond)) { FAIL(msg); return; } \
} while(0)

typedef struct {
    bool started;
    bool stopped;
    bool paused;
    bool resumed;
    bool finished;
    bool correct;
    bool incorrect;
    bool backspace;
    bool timeout;
    bool error;
    int call_count;
} CallbackFlags;

static void reset_flags(CallbackFlags* f) {
    memset(f, 0, sizeof(CallbackFlags));
}

static void on_start(Engine* e, void* data) {
    (void)e;
    ((CallbackFlags*)data)->started = true;
    ((CallbackFlags*)data)->call_count++;
}

static void on_stop(Engine* e, void* data) {
    (void)e;
    ((CallbackFlags*)data)->stopped = true;
    ((CallbackFlags*)data)->call_count++;
}

static void on_pause(Engine* e, void* data) {
    (void)e;
    ((CallbackFlags*)data)->paused = true;
    ((CallbackFlags*)data)->call_count++;
}

static void on_resume(Engine* e, void* data) {
    (void)e;
    ((CallbackFlags*)data)->resumed = true;
    ((CallbackFlags*)data)->call_count++;
}

static void on_finish(Engine* e, void* data) {
    (void)e;
    ((CallbackFlags*)data)->finished = true;
    ((CallbackFlags*)data)->call_count++;
}

static void on_correct(Engine* e, void* data) {
    (void)e;
    ((CallbackFlags*)data)->correct = true;
    ((CallbackFlags*)data)->call_count++;
}

static void on_incorrect(Engine* e, void* data) {
    (void)e;
    ((CallbackFlags*)data)->incorrect = true;
    ((CallbackFlags*)data)->call_count++;
}

static void on_backspace(Engine* e, void* data) {
    (void)e;
    ((CallbackFlags*)data)->backspace = true;
    ((CallbackFlags*)data)->call_count++;
}

static void on_timeout(Engine* e, void* data) {
    (void)e;
    ((CallbackFlags*)data)->timeout = true;
    ((CallbackFlags*)data)->call_count++;
}

static void test_create_destroy(void) {
    TEST("Engine creation and destruction");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    ASSERT(engineGetMode(e) == StrictMode, "mode should be StrictMode");
    ASSERT(engineGetTimeout(e) == 0, "timeout should be 0");
    ASSERT(engineIsIdle(e), "engine should be idle after creation");
    engineDestroy(e);
    PASS();
}

static void test_mode(void) {
    TEST("Mode get/set");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    ASSERT(engineGetMode(e) == StrictMode, "initial mode should be StrictMode");
    engineSetMode(e, FlowMode);
    ASSERT(engineGetMode(e) == FlowMode, "mode should be FlowMode after set");
    engineSetMode(e, StrictMode);
    ASSERT(engineGetMode(e) == StrictMode, "mode should be StrictMode after set back");
    engineDestroy(e);
    PASS();
}

static void test_timeout(void) {
    TEST("Timeout get/set");
    Engine* e = engineCreate(StrictMode, 30);
    ASSERT(e != NULL, "engineCreate returned NULL");
    ASSERT(engineGetTimeout(e) == 30, "initial timeout should be 30");
    engineSetTimeout(e, 60);
    ASSERT(engineGetTimeout(e) == 60, "timeout should be 60 after set");
    engineDestroy(e);
    PASS();
}

static void test_state_transitions(void) {
    TEST("State transitions: start -> stop");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    ASSERT(engineIsIdle(e), "should be idle initially");
    
    engineStart(e);
    ASSERT(engineIsRunning(e), "should be running after start");
    ASSERT(!engineIsIdle(e), "should not be idle after start");
    
    engineStop(e);
    ASSERT(!engineIsIdle(e), "should not be idle (has stop cause)");
    ASSERT(engineIsStopped(e), "should be stopped (user stop)");
    engineDestroy(e);
    PASS();
}

static void test_pause_resume(void) {
    TEST("State transitions: start -> pause -> resume -> stop");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    engineStart(e);
    ASSERT(engineIsRunning(e), "should be running after start");
    
    enginePause(e);
    ASSERT(engineIsPaused(e), "should be paused after pause");
    ASSERT(!engineIsRunning(e), "should not be running after pause");
    
    engineResume(e);
    ASSERT(engineIsRunning(e), "should be running after resume");
    
    engineStop(e);
    ASSERT(engineIsStopped(e), "should be stopped");
    engineDestroy(e);
    PASS();
}

static void test_reset(void) {
    TEST("State transitions: start -> reset");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    engineStart(e);
    ASSERT(engineIsRunning(e), "should be running after start");
    
    engineReset(e);
    ASSERT(engineIsIdle(e), "should be idle after reset");
    ASSERT(!engineIsStopped(e), "should not be stopped after reset");
    ASSERT(!engineIsCompleted(e), "should not be completed after reset");
    engineDestroy(e);
    PASS();
}

static void test_strict_correct_key(void) {
    TEST("Strict mode: correct key advances");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    engineStart(e);
    ASSERT(engineIsRunning(e), "should be running");
    
    engineKeyPress(e, 'T');
    
    SessionStats stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 1, "totalKeystrokes should be 1");
    ASSERT(stats.correctKeystrokes == 1, "correctKeystrokes should be 1");
    ASSERT(stats.accuracy == 100.0, "accuracy should be 100%%");
    
    engineDestroy(e);
    PASS();
}

static void test_strict_incorrect_key(void) {
    TEST("Strict mode: incorrect key does not advance");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    engineStart(e);
    ASSERT(engineIsRunning(e), "should be running");
    
    engineKeyPress(e, 'X');
    
    SessionStats stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 1, "totalKeystrokes should be 1");
    ASSERT(stats.correctKeystrokes == 0, "correctKeystrokes should be 0");
    ASSERT(stats.accuracy == 0.0, "accuracy should be 0%%");
    
    engineKeyPress(e, 'T');
    stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 2, "totalKeystrokes should be 2");
    ASSERT(stats.correctKeystrokes == 1, "correctKeystrokes should be 1");
    
    engineDestroy(e);
    PASS();
}

static void test_strict_backspace_disabled(void) {
    TEST("Strict mode: backspace is disabled");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    engineStart(e);
    ASSERT(engineIsRunning(e), "should be running");
    
    engineKeyPress(e, 'T');
    engineBackspacePress(e);
    
    SessionStats stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 1, "totalKeystrokes should still be 1");
    ASSERT(stats.correctKeystrokes == 1, "correctKeystrokes should still be 1");
    
    engineDestroy(e);
    PASS();
}

static void test_flow_correct_key(void) {
    TEST("Flow mode: correct key advances");
    Engine* e = engineCreate(FlowMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    engineStart(e);
    ASSERT(engineIsRunning(e), "should be running");
    
    engineKeyPress(e, 'T');
    
    SessionStats stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 1, "totalKeystrokes should be 1");
    ASSERT(stats.correctKeystrokes == 1, "correctKeystrokes should be 1");
    
    engineDestroy(e);
    PASS();
}

static void test_flow_incorrect_key_advances(void) {
    TEST("Flow mode: incorrect key still advances");
    Engine* e = engineCreate(FlowMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    engineStart(e);
    ASSERT(engineIsRunning(e), "should be running");
    
    engineKeyPress(e, 'X');
    
    SessionStats stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 1, "totalKeystrokes should be 1");
    ASSERT(stats.correctKeystrokes == 0, "correctKeystrokes should be 0");
    
    engineKeyPress(e, 'h');
    stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 2, "totalKeystrokes should be 2");
    ASSERT(stats.correctKeystrokes == 1, "correctKeystrokes should be 1");
    
    engineDestroy(e);
    PASS();
}

static void test_flow_backspace_correct(void) {
    TEST("Flow mode: backspace over correct key");
    Engine* e = engineCreate(FlowMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    engineStart(e);
    ASSERT(engineIsRunning(e), "should be running");
    
    engineKeyPress(e, 'T');
    engineKeyPress(e, 'h');
    
    SessionStats stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 2, "totalKeystrokes should be 2");
    ASSERT(stats.correctKeystrokes == 2, "correctKeystrokes should be 2");
    
    engineBackspacePress(e);
    
    stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 2, "totalKeystrokes should still be 2");
    ASSERT(stats.correctKeystrokes == 1, "correctKeystrokes should be 1 after undoing correct char");
    
    engineDestroy(e);
    PASS();
}

static void test_flow_backspace_incorrect(void) {
    TEST("Flow mode: backspace over incorrect key");
    Engine* e = engineCreate(FlowMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    engineStart(e);
    ASSERT(engineIsRunning(e), "should be running");
    
    engineKeyPress(e, 'X');
    
    SessionStats stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 1, "totalKeystrokes should be 1");
    ASSERT(stats.correctKeystrokes == 0, "correctKeystrokes should be 0");
    
    engineBackspacePress(e);
    
    stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 1, "totalKeystrokes should still be 1");
    ASSERT(stats.correctKeystrokes == 0, "correctKeystrokes should still be 0");
    
    engineKeyPress(e, 'T');
    stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 2, "totalKeystrokes should be 2");
    ASSERT(stats.correctKeystrokes == 1, "correctKeystrokes should be 1");
    
    engineDestroy(e);
    PASS();
}

static void test_session_complete_strict(void) {
    TEST("Strict mode: session completion fires events");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    CallbackFlags flags;
    reset_flags(&flags);
    
    engineOnFinished(e, on_finish, &flags);
    engineOnStopped(e, on_stop, &flags);
    
    engineStart(e);
    ASSERT(engineIsRunning(e), "should be running");
    
    const char* text = "The quick brown fox jumps over the lazy dog.";
    for (const char* p = text; *p; p++) {
        engineKeyPress(e, *p);
    }
    
    ASSERT(!engineIsIdle(e), "should not be idle (has finish cause)");
    ASSERT(engineIsCompleted(e), "should be completed");
    ASSERT(flags.finished, "FINISHED callback should have fired");
    ASSERT(flags.stopped, "STOPPED callback should have fired");
    
    engineDestroy(e);
    PASS();
}

static void test_session_complete_flow(void) {
    TEST("Flow mode: session completion fires events");
    Engine* e = engineCreate(FlowMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    CallbackFlags flags;
    reset_flags(&flags);
    
    engineOnFinished(e, on_finish, &flags);
    engineOnStopped(e, on_stop, &flags);
    
    engineStart(e);
    ASSERT(engineIsRunning(e), "should be running");
    
    const char* text = "The quick brown fox jumps over the lazy dog.";
    for (const char* p = text; *p; p++) {
        engineKeyPress(e, *p);
    }
    
    ASSERT(!engineIsIdle(e), "should not be idle (has finish cause)");
    ASSERT(engineIsCompleted(e), "should be completed");
    ASSERT(flags.finished, "FINISHED callback should have fired");
    ASSERT(flags.stopped, "STOPPED callback should have fired");
    
    engineDestroy(e);
    PASS();
}

static void test_callbacks(void) {
    TEST("Callback system: all lifecycle events fire");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    CallbackFlags flags;
    reset_flags(&flags);
    
    engineOnStarted(e, on_start, &flags);
    engineOnStopped(e, on_stop, &flags);
    engineOnPaused(e, on_pause, &flags);
    engineOnResumed(e, on_resume, &flags);
    engineOnCorrectKeystroke(e, on_correct, &flags);
    engineOnIncorrectKeystroke(e, on_incorrect, &flags);
    engineOnBackspace(e, on_backspace, &flags);
    
    engineStart(e);
    ASSERT(flags.started, "STARTED callback should fire");
    ASSERT(flags.call_count == 1, "call_count should be 1 after start");
    
    engineKeyPress(e, 'T');
    ASSERT(flags.correct, "CORRECT_KEYSTROKE callback should fire");
    
    engineKeyPress(e, 'X');
    ASSERT(flags.incorrect, "INCORRECT_KEYSTROKE callback should fire");
    
    enginePause(e);
    ASSERT(flags.paused, "PAUSED callback should fire");
    
    engineResume(e);
    ASSERT(flags.resumed, "RESUMED callback should fire");
    
    engineStop(e);
    ASSERT(flags.stopped, "STOPPED callback should fire");
    
    engineDestroy(e);
    PASS();
}

static void test_error_already_running(void) {
    TEST("Error handling: start when already running");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    engineStart(e);
    engineStart(e);
    
    char buf[64];
    engineErrorToString(ENGINE_ERROR_ALREADY_RUNNING, buf, sizeof(buf));
    ASSERT(strcmp(buf, "Already Running") == 0, "error string should be 'Already Running'");
    
    engineDestroy(e);
    PASS();
}

static void test_error_not_running(void) {
    TEST("Error handling: stop when not running");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    engineStop(e);
    
    char buf[64];
    engineErrorToString(ENGINE_ERROR_NOT_RUNNING, buf, sizeof(buf));
    ASSERT(strcmp(buf, "Not Running") == 0, "error string should be 'Not Running'");
    
    engineDestroy(e);
    PASS();
}

static void test_error_to_string_all(void) {
    TEST("Error: all error codes convert to strings");
    char buf[64];
    
    engineErrorToString(ENGINE_ERROR_NONE, buf, sizeof(buf));
    ASSERT(strcmp(buf, "No Error") == 0, "NONE string");
    
    engineErrorToString(ENGINE_ERROR_INVALID_MODE, buf, sizeof(buf));
    ASSERT(strcmp(buf, "Invalid Mode") == 0, "INVALID_MODE string");
    
    engineErrorToString(ENGINE_ERROR_INVALID_TIMEOUT, buf, sizeof(buf));
    ASSERT(strcmp(buf, "Invalid Timeout") == 0, "INVALID_TIMEOUT string");
    
    engineErrorToString(ENGINE_ERROR_UNKNOWN, buf, sizeof(buf));
    ASSERT(strcmp(buf, "Unknown Error") == 0, "UNKNOWN string");
    
    PASS();
}

static void test_pause_error_path(void) {
    TEST("Error handling: pause when not running");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    ASSERT(engineIsIdle(e), "should be idle");
    enginePause(e);
    ASSERT(engineIsIdle(e), "should still be idle after pause on non-running engine");
    
    engineDestroy(e);
    PASS();
}

static void test_resume_error_path(void) {
    TEST("Error handling: resume when not paused");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    engineStart(e);
    ASSERT(engineIsRunning(e), "should be running");
    engineResume(e);
    ASSERT(engineIsRunning(e), "should still be running after resume on non-paused engine");
    
    engineStop(e);
    engineDestroy(e);
    PASS();
}

static void test_null_safety(void) {
    TEST("NULL safety: all functions handle NULL gracefully");
    
    engineDestroy(NULL);
    engineSetMode(NULL, StrictMode);
    engineGetMode(NULL);
    engineSetTimeout(NULL, 0);
    engineGetTimeout(NULL);
    engineStart(NULL);
    engineStop(NULL);
    enginePause(NULL);
    engineResume(NULL);
    engineReset(NULL);
    engineKeyPress(NULL, 'a');
    engineBackspacePress(NULL);
    engineGetStateInfo(NULL);
    engineGetStats(NULL);
    engineIsRunning(NULL);
    engineIsPaused(NULL);
    engineIsIdle(NULL);
    engineIsError(NULL);
    engineIsCompleted(NULL);
    engineIsTimedOut(NULL);
    engineIsStopped(NULL);
    engineOnStarted(NULL, NULL, NULL);
    engineOnStopped(NULL, NULL, NULL);
    engineOnTimeout(NULL, NULL, NULL);
    engineOnFinished(NULL, NULL, NULL);
    engineOnBackspace(NULL, NULL, NULL);
    engineDisconnect(NULL, ENGINE_EVENT_STARTED, 0);
    engineClearEvent(NULL, ENGINE_EVENT_STARTED);
    engineSetAutoSave(NULL, NULL, false);
    
    PASS();
}

static void test_stats_accuracy(void) {
    TEST("Stats: accuracy calculation");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    engineStart(e);
    
    engineKeyPress(e, 'T');
    engineKeyPress(e, 'X');
    engineKeyPress(e, 'h');
    engineKeyPress(e, 'e');
    
    SessionStats stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 4, "totalKeystrokes should be 4");
    ASSERT(stats.correctKeystrokes == 3, "correctKeystrokes should be 3");
    ASSERT(stats.accuracy == 75.0, "accuracy should be 75.0");
    
    engineDestroy(e);
    PASS();
}

static void test_stats_wpm(void) {
    TEST("Stats: WPM calculation");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    engineStart(e);
    
    engineKeyPress(e, 'T');
    engineKeyPress(e, 'h');
    engineKeyPress(e, 'e');
    engineKeyPress(e, ' ');
    
    SessionStats stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 4, "totalKeystrokes should be 4");
    ASSERT(stats.correctKeystrokes == 4, "correctKeystrokes should be 4");
    ASSERT(stats.accuracy == 100.0, "accuracy should be 100%%");
    
    engineDestroy(e);
    PASS();
}

static void test_event_to_string(void) {
    TEST("Event to string conversion");
    char buf[64];
    
    engineEventToString(ENGINE_EVENT_NONE, buf, sizeof(buf));
    ASSERT(strcmp(buf, "No Event") == 0, "NONE string");
    
    engineEventToString(ENGINE_EVENT_STARTED, buf, sizeof(buf));
    ASSERT(strcmp(buf, "Started") == 0, "STARTED string");
    
    engineEventToString(ENGINE_EVENT_FINISHED, buf, sizeof(buf));
    ASSERT(strcmp(buf, "Finished") == 0, "FINISHED string");
    
    engineEventToString(ENGINE_EVENT_CORRECT_KEYSTROKE, buf, sizeof(buf));
    ASSERT(strcmp(buf, "Correct Keystroke") == 0, "CORRECT_KEYSTROKE string");
    
    engineEventToString(ENGINE_EVENT_INCORRECT_KEYSTROKE, buf, sizeof(buf));
    ASSERT(strcmp(buf, "Incorrect Keystroke") == 0, "INCORRECT_KEYSTROKE string");
    
    engineEventToString(ENGINE_EVENT_STOPPED, buf, sizeof(buf));
    ASSERT(strcmp(buf, "Stopped") == 0, "STOPPED string");
    
    engineEventToString(ENGINE_EVENT_PAUSED, buf, sizeof(buf));
    ASSERT(strcmp(buf, "Paused") == 0, "PAUSED string");
    
    engineEventToString(ENGINE_EVENT_RESUMED, buf, sizeof(buf));
    ASSERT(strcmp(buf, "Resumed") == 0, "RESUMED string");
    
    engineEventToString(ENGINE_EVENT_TIMEOUT, buf, sizeof(buf));
    ASSERT(strcmp(buf, "Timeout") == 0, "TIMEOUT string");
    
    engineEventToString(ENGINE_EVENT_BACKSPACE, buf, sizeof(buf));
    ASSERT(strcmp(buf, "Backspace Pressed") == 0, "BACKSPACE string");
    
    engineEventToString(ENGINE_EVENT_SEGMENT_COMPLETED, buf, sizeof(buf));
    ASSERT(strcmp(buf, "Segment Completed") == 0, "SEGMENT_COMPLETED string");
    
    engineEventToString(ENGINE_EVENT_ERROR, buf, sizeof(buf));
    ASSERT(strcmp(buf, "Error Occurred") == 0, "ERROR string");
    
    PASS();
}

static void test_state_info(void) {
    TEST("State info: engineGetStateInfo");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    EngineStateInfo info = engineGetStateInfo(e);
    ASSERT(info.state == ENGINE_IDLE, "state should be IDLE");
    ASSERT(info.stopCause == ENGINE_STOP_CAUSE_NONE, "stopCause should be NONE");
    
    engineStart(e);
    info = engineGetStateInfo(e);
    ASSERT(info.state == ENGINE_RUNNING, "state should be RUNNING");
    
    engineStop(e);
    info = engineGetStateInfo(e);
    ASSERT(info.state == ENGINE_IDLE, "state should be IDLE");
    ASSERT(info.stopCause == ENGINE_STOP_CAUSE_USER, "stopCause should be USER");
    
    info = engineGetStateInfo(NULL);
    ASSERT(info.state == ENGINE_ERROR, "NULL engine should return ERROR state");
    ASSERT(info.stopCause == ENGINE_STOP_CAUSE_ERROR, "NULL engine should return ERROR stopCause");
    
    engineDestroy(e);
    PASS();
}

static void test_multiple_callbacks(void) {
    TEST("Multiple callbacks for same event");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    CallbackFlags flags1, flags2;
    reset_flags(&flags1);
    reset_flags(&flags2);
    
    int reg1 = engineOnStarted(e, on_start, &flags1);
    int reg2 = engineOnStarted(e, on_start, &flags2);
    ASSERT(reg1 >= 0, "first registration should succeed");
    ASSERT(reg2 >= 0, "second registration should succeed");
    
    engineStart(e);
    ASSERT(flags1.started, "first callback should fire");
    ASSERT(flags2.started, "second callback should fire");
    ASSERT(flags1.call_count == 1, "first callback call_count should be 1");
    ASSERT(flags2.call_count == 1, "second callback call_count should be 1");
    
    engineDestroy(e);
    PASS();
}

static void test_max_callbacks(void) {
    TEST("Max callbacks limit per event");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    CallbackFlags flags;
    reset_flags(&flags);
    
    for (int i = 0; i < 5; i++) {
        int reg = engineOnStarted(e, on_start, &flags);
        ASSERT(reg >= 0, "registration should succeed");
    }
    
    int reg = engineOnStarted(e, on_start, &flags);
    ASSERT(reg < 0, "6th registration should fail");
    
    engineDestroy(e);
    PASS();
}

static void test_multi_event_slots(void) {
    TEST("Different event types have independent slot limits");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    CallbackFlags flags;
    reset_flags(&flags);
    
    for (int i = 0; i < 5; i++) {
        ASSERT(engineOnStarted(e, on_start, &flags) >= 0, "fill STARTED slots");
    }
    ASSERT(engineOnStarted(e, on_start, &flags) < 0, "STARTED full");
    
    ASSERT(engineOnPaused(e, on_pause, &flags) >= 0, "PAUSED should still accept");
    ASSERT(engineOnStopped(e, on_stop, &flags) >= 0, "STOPPED should still accept");
    
    engineDestroy(e);
    PASS();
}

static void test_signal_disconnect(void) {
    TEST("Signal: disconnect removes callback");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    CallbackFlags flags;
    reset_flags(&flags);
    
    int slotId = engineOnStarted(e, on_start, &flags);
    ASSERT(slotId >= 0, "registration should succeed");
    
    engineDisconnect(e, ENGINE_EVENT_STARTED, slotId);
    engineStart(e);
    ASSERT(!flags.started, "callback should NOT fire after disconnect");
    ASSERT(flags.call_count == 0, "call_count should be 0");
    
    engineDestroy(e);
    PASS();
}

static void test_signal_clear(void) {
    TEST("Signal: clear removes all callbacks for an event");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    CallbackFlags flags1, flags2;
    reset_flags(&flags1);
    reset_flags(&flags2);
    
    engineOnStarted(e, on_start, &flags1);
    engineOnStarted(e, on_start, &flags2);
    
    engineClearEvent(e, ENGINE_EVENT_STARTED);
    engineStart(e);
    ASSERT(!flags1.started, "first callback should NOT fire after clear");
    ASSERT(!flags2.started, "second callback should NOT fire after clear");
    
    engineDestroy(e);
    PASS();
}

static void test_signal_clear_isolation(void) {
    TEST("Signal: clearing one event does not affect others");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    CallbackFlags flags;
    reset_flags(&flags);
    
    engineOnStarted(e, on_start, &flags);
    engineOnStopped(e, on_stop, &flags);
    
    engineClearEvent(e, ENGINE_EVENT_STARTED);
    engineStart(e);
    ASSERT(!flags.started, "STARTED callback should NOT fire after clear");
    ASSERT(flags.call_count == 0, "call_count should be 0 after start");
    
    engineStop(e);
    ASSERT(flags.stopped, "STOPPED callback should still fire");
    
    engineDestroy(e);
    PASS();
}

static void test_flow_backspace_retry(void) {
    TEST("Flow mode: backspace incorrect, retype correct");
    Engine* e = engineCreate(FlowMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    engineStart(e);
    ASSERT(engineIsRunning(e), "should be running");
    
    engineKeyPress(e, 'X');
    SessionStats stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 1, "totalKeystrokes should be 1");
    ASSERT(stats.correctKeystrokes == 0, "correctKeystrokes should be 0");
    
    engineBackspacePress(e);
    stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 1, "totalKeystrokes should still be 1");
    ASSERT(stats.correctKeystrokes == 0, "correctKeystrokes should still be 0");
    
    engineKeyPress(e, 'T');
    stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 2, "totalKeystrokes should be 2");
    ASSERT(stats.correctKeystrokes == 1, "correctKeystrokes should be 1");
    ASSERT(stats.accuracy == 50.0, "accuracy should be 50.0");
    
    engineDestroy(e);
    PASS();
}

static void test_timeout_triggers(void) {
    TEST("Timeout: triggers after configured duration");
    Engine* e = engineCreate(StrictMode, 1);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    CallbackFlags flags;
    reset_flags(&flags);
    
    engineOnTimeout(e, on_timeout, &flags);
    engineOnStopped(e, on_stop, &flags);
    
    engineStart(e);
    ASSERT(engineIsRunning(e), "should be running");
    
#ifdef _WIN32
    Sleep(1100);
#else
    struct timespec ts = {1, 100000000L};
    nanosleep(&ts, NULL);
#endif
    
    engineKeyPress(e, 'T');
    
    ASSERT(engineIsTimedOut(e), "should be timed out");
    ASSERT(flags.timeout, "TIMEOUT callback should have fired");
    ASSERT(flags.stopped, "STOPPED callback should have fired");
    
    SessionStats stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 0, "key should not have been processed after timeout");
    
    engineDestroy(e);
    PASS();
}

static void test_timeout_zero_disabled(void) {
    TEST("Timeout: zero timeout means no timeout");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    engineStart(e);
    ASSERT(engineIsRunning(e), "should be running");
    
    engineKeyPress(e, 'T');
    engineKeyPress(e, 'h');
    engineKeyPress(e, 'e');
    
    ASSERT(engineIsRunning(e), "should still be running (no timeout)");
    
    engineStop(e);
    engineDestroy(e);
    PASS();
}

static void test_timeout_pause_does_not_accumulate(void) {
    TEST("Timeout: paused time does not count toward timeout");
    Engine* e = engineCreate(StrictMode, 1);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    engineStart(e);
    ASSERT(engineIsRunning(e), "should be running");
    
    engineKeyPress(e, 'T');
    enginePause(e);
    ASSERT(engineIsPaused(e), "should be paused");
    
#ifdef _WIN32
    Sleep(1500);
#else
    struct timespec ts = {1, 500000000L};
    nanosleep(&ts, NULL);
#endif
    
    engineResume(e);
    ASSERT(engineIsRunning(e), "should still be running (paused time not counted)");
    
    engineKeyPress(e, 'h');
    ASSERT(engineIsRunning(e), "should still be running after resume + key");
    
    engineStop(e);
    engineDestroy(e);
    PASS();
}

static void test_timeout_backspace_checks_timeout(void) {
    TEST("Timeout: backspace also triggers timeout check");
    Engine* e = engineCreate(FlowMode, 1);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    CallbackFlags flags;
    reset_flags(&flags);
    
    engineOnTimeout(e, on_timeout, &flags);
    
    engineStart(e);
    ASSERT(engineIsRunning(e), "should be running");
    
    engineKeyPress(e, 'T');
    engineKeyPress(e, 'h');
    
#ifdef _WIN32
    Sleep(1100);
#else
    struct timespec ts = {1, 100000000L};
    nanosleep(&ts, NULL);
#endif
    
    engineBackspacePress(e);
    
    ASSERT(engineIsTimedOut(e), "should be timed out after backspace");
    ASSERT(flags.timeout, "TIMEOUT callback should have fired");
    
    SessionStats stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 2, "keystrokes should still be 2 (backspace not processed)");
    
    engineDestroy(e);
    PASS();
}

static void test_auto_save_session(void) {
    TEST("Auto-save: saves session on completion");
    Engine* e = engineCreate(FlowMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");

    Repository* repo = repositoryCreate("test_autosave.db");
    ASSERT(repo != NULL, "repositoryCreate returned NULL");

    engineSetAutoSave(e, repo, true);
    engineStart(e);
    ASSERT(engineIsRunning(e), "should be running");

    const char* text = "The quick brown fox jumps over the lazy dog.";
    for (const char* p = text; *p; p++) {
        engineKeyPress(e, *p);
    }

    ASSERT(engineIsCompleted(e), "should be completed");

    size_t count = 0;
    SessionData* sessions = repositoryGetAll(repo, &count);
    ASSERT(count == 1, "should have 1 saved session");
    ASSERT(sessions != NULL, "sessions should not be NULL");
    ASSERT(sessions[0].totalChars == 44, "totalChars should be 44");
    ASSERT(sessions[0].correctChars == 44, "correctChars should be 44");
    ASSERT(sessions[0].accuracy == 100.0, "accuracy should be 100.0");
    free(sessions);

    remove("test_autosave.db");
    engineDestroy(e);
    repositoryDestroy(repo);
    PASS();
}

static void test_stats_before_start(void) {
    TEST("Stats: get stats before starting");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    SessionStats stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 0, "totalKeystrokes should be 0");
    ASSERT(stats.correctKeystrokes == 0, "correctKeystrokes should be 0");
    ASSERT(stats.accuracy == 100.0, "accuracy should default to 100.0");
    ASSERT(stats.wpm == 0.0, "wpm should be 0.0");
    ASSERT(stats.wpmRaw == 0.0, "wpmRaw should be 0.0");
    ASSERT(stats.durationMs == 0, "durationMs should be 0");
    
    engineDestroy(e);
    PASS();
}

static void test_stats_null_engine(void) {
    TEST("Stats: get stats with NULL engine");
    SessionStats stats = engineGetStats(NULL);
    ASSERT(stats.totalKeystrokes == 0, "totalKeystrokes should be 0");
    ASSERT(stats.correctKeystrokes == 0, "correctKeystrokes should be 0");
    ASSERT(stats.durationMs == 0, "durationMs should be 0");
    PASS();
}

int main(void) {
    printf("=== ctypr Engine Test Suite ===\n\n");
    
    test_create_destroy();
    test_mode();
    test_timeout();
    
    test_state_transitions();
    test_pause_resume();
    test_reset();
    
    test_strict_correct_key();
    test_strict_incorrect_key();
    test_strict_backspace_disabled();
    
    test_flow_correct_key();
    test_flow_incorrect_key_advances();
    
    test_flow_backspace_correct();
    test_flow_backspace_incorrect();
    test_flow_backspace_retry();
    
    test_session_complete_strict();
    test_session_complete_flow();
    
    test_callbacks();
    test_multiple_callbacks();
    test_max_callbacks();
    test_multi_event_slots();
    test_signal_disconnect();
    test_signal_clear();
    test_signal_clear_isolation();
    
    test_stats_accuracy();
    test_stats_wpm();
    test_stats_before_start();
    test_stats_null_engine();
    
    test_error_already_running();
    test_error_not_running();
    test_error_to_string_all();
    test_pause_error_path();
    test_resume_error_path();
    
    test_event_to_string();
    test_state_info();
    
    test_null_safety();
    
    test_timeout_triggers();
    test_timeout_zero_disabled();
    test_timeout_pause_does_not_accumulate();
    test_timeout_backspace_checks_timeout();
    test_auto_save_session();
    
    printf("\n=== Results: %d passed, %d failed, %d total ===\n",
           tests_passed, tests_failed, test_count);
    
    return tests_failed > 0 ? 1 : 0;
}
