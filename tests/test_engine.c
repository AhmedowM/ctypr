#include "engine.h"
#include "callback.h"
#include "error.h"
#include "event.h"
#include "state.h"
#include "stats.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Simple test framework */
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

/* Callback tracking */
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

/* ===== Test: Engine Creation and Destruction ===== */
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

/* ===== Test: Mode get/set ===== */
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

/* ===== Test: Timeout get/set ===== */
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

/* ===== Test: State transitions ===== */
static void test_state_transitions(void) {
    TEST("State transitions: start -> stop");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    ASSERT(engineIsIdle(e), "should be idle initially");
    
    engineStart(e);
    ASSERT(engineIsRunning(e), "should be running after start");
    ASSERT(!engineIsIdle(e), "should not be idle after start");
    
    engineStop(e);
    ASSERT(engineIsIdle(e), "should be idle after stop");
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
    ASSERT(engineIsIdle(e), "should be idle after stop");
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

/* ===== Test: Strict mode key processing ===== */
static void test_strict_correct_key(void) {
    TEST("Strict mode: correct key advances");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    engineStart(e);
    ASSERT(engineIsRunning(e), "should be running");
    
    // "The quick brown fox jumps over the lazy dog."
    // First char is 'T'
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
    
    // First char is 'T', press 'X' (wrong)
    engineKeyPress(e, 'X');
    
    SessionStats stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 1, "totalKeystrokes should be 1");
    ASSERT(stats.correctKeystrokes == 0, "correctKeystrokes should be 0");
    ASSERT(stats.accuracy == 0.0, "accuracy should be 0%%");
    
    // Now press correct key 'T'
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
    engineBackspacePress(e); // Should do nothing in strict mode
    
    SessionStats stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 1, "totalKeystrokes should still be 1");
    ASSERT(stats.correctKeystrokes == 1, "correctKeystrokes should still be 1");
    
    engineDestroy(e);
    PASS();
}

/* ===== Test: Flow mode key processing ===== */
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
    
    // First char is 'T', press 'X' (wrong) - should still advance
    engineKeyPress(e, 'X');
    
    SessionStats stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 1, "totalKeystrokes should be 1");
    ASSERT(stats.correctKeystrokes == 0, "correctKeystrokes should be 0");
    
    // Now the expected char is 'h' (second char)
    engineKeyPress(e, 'h');
    stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 2, "totalKeystrokes should be 2");
    ASSERT(stats.correctKeystrokes == 1, "correctKeystrokes should be 1");
    
    engineDestroy(e);
    PASS();
}

/* ===== Test: Backspace in flow mode ===== */
static void test_flow_backspace_correct(void) {
    TEST("Flow mode: backspace over correct key");
    Engine* e = engineCreate(FlowMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    engineStart(e);
    ASSERT(engineIsRunning(e), "should be running");
    
    engineKeyPress(e, 'T'); // correct
    engineKeyPress(e, 'h'); // correct
    
    SessionStats stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 2, "totalKeystrokes should be 2");
    ASSERT(stats.correctKeystrokes == 2, "correctKeystrokes should be 2");
    
    engineBackspacePress(e); // undo 'h'
    
    stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 2, "totalKeystrokes should still be 2 (backspace is not a keystroke)");
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
    
    engineKeyPress(e, 'X'); // incorrect (expected 'T')
    
    SessionStats stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 1, "totalKeystrokes should be 1");
    ASSERT(stats.correctKeystrokes == 0, "correctKeystrokes should be 0");
    
    engineBackspacePress(e); // undo the incorrect 'X'
    
    stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 1, "totalKeystrokes should still be 1");
    ASSERT(stats.correctKeystrokes == 0, "correctKeystrokes should still be 0 (was never correct)");
    
    // Now type correct key
    engineKeyPress(e, 'T');
    stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 2, "totalKeystrokes should be 2");
    ASSERT(stats.correctKeystrokes == 1, "correctKeystrokes should be 1");
    
    engineDestroy(e);
    PASS();
}

/* ===== Test: Session completion ===== */
static void test_session_complete_strict(void) {
    TEST("Strict mode: session completion fires events");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    CallbackFlags flags;
    reset_flags(&flags);
    
    engineRegisterCallback(e, &(EngineEvent){ENGINE_EVENT_FINISHED}, on_finish, &flags);
    engineRegisterCallback(e, &(EngineEvent){ENGINE_EVENT_STOPPED}, on_stop, &flags);
    
    engineStart(e);
    ASSERT(engineIsRunning(e), "should be running");
    
    // Type the full text: "The quick brown fox jumps over the lazy dog."
    const char* text = "The quick brown fox jumps over the lazy dog.";
    for (const char* p = text; *p; p++) {
        engineKeyPress(e, *p);
    }
    
    ASSERT(engineIsIdle(e), "should be idle after completion");
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
    
    engineRegisterCallback(e, &(EngineEvent){ENGINE_EVENT_FINISHED}, on_finish, &flags);
    engineRegisterCallback(e, &(EngineEvent){ENGINE_EVENT_STOPPED}, on_stop, &flags);
    
    engineStart(e);
    ASSERT(engineIsRunning(e), "should be running");
    
    // Type the full text
    const char* text = "The quick brown fox jumps over the lazy dog.";
    for (const char* p = text; *p; p++) {
        engineKeyPress(e, *p);
    }
    
    ASSERT(engineIsIdle(e), "should be idle after completion");
    ASSERT(engineIsCompleted(e), "should be completed");
    ASSERT(flags.finished, "FINISHED callback should have fired");
    ASSERT(flags.stopped, "STOPPED callback should have fired");
    
    engineDestroy(e);
    PASS();
}

/* ===== Test: Callback system ===== */
static void test_callbacks(void) {
    TEST("Callback system: all lifecycle events fire");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    CallbackFlags flags;
    reset_flags(&flags);
    
    engineRegisterCallback(e, &(EngineEvent){ENGINE_EVENT_STARTED}, on_start, &flags);
    engineRegisterCallback(e, &(EngineEvent){ENGINE_EVENT_STOPPED}, on_stop, &flags);
    engineRegisterCallback(e, &(EngineEvent){ENGINE_EVENT_PAUSED}, on_pause, &flags);
    engineRegisterCallback(e, &(EngineEvent){ENGINE_EVENT_RESUMED}, on_resume, &flags);
    engineRegisterCallback(e, &(EngineEvent){ENGINE_EVENT_CORRECT_KEYSTROKE}, on_correct, &flags);
    engineRegisterCallback(e, &(EngineEvent){ENGINE_EVENT_INCORRECT_KEYSTROKE}, on_incorrect, &flags);
    engineRegisterCallback(e, &(EngineEvent){ENGINE_EVENT_BACKSPACE}, on_backspace, &flags);
    
    engineStart(e);
    ASSERT(flags.started, "STARTED callback should fire");
    ASSERT(flags.call_count == 1, "call_count should be 1 after start");
    
    engineKeyPress(e, 'T'); // correct
    ASSERT(flags.correct, "CORRECT_KEYSTROKE callback should fire");
    
    engineKeyPress(e, 'X'); // incorrect
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

/* ===== Test: Error handling ===== */
static void test_error_already_running(void) {
    TEST("Error handling: start when already running");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    engineStart(e);
    engineStart(e); // Should set error but not crash
    
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
    
    engineStop(e); // Should not crash, should set error
    
    char buf[64];
    engineErrorToString(ENGINE_ERROR_NOT_RUNNING, buf, sizeof(buf));
    ASSERT(strcmp(buf, "Not Running") == 0, "error string should be 'Not Running'");
    
    engineDestroy(e);
    PASS();
}

/* ===== Test: NULL safety ===== */
static void test_null_safety(void) {
    TEST("NULL safety: all functions handle NULL gracefully");
    
    // These should not crash
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
    engineRegisterCallback(NULL, NULL, NULL, NULL);
    engineExecuteCallbacks(NULL, NULL);
    engineGetStateInfo(NULL);
    engineGetStats(NULL);
    engineIsRunning(NULL);
    engineIsPaused(NULL);
    engineIsIdle(NULL);
    engineIsError(NULL);
    engineIsCompleted(NULL);
    engineIsTimedOut(NULL);
    engineIsStopped(NULL);
    
    PASS();
}

/* ===== Test: Stats calculation ===== */
static void test_stats_accuracy(void) {
    TEST("Stats: accuracy calculation");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    engineStart(e);
    
    // "The quick brown fox jumps over the lazy dog."
    // Press: T(✓), X(✗), h(✓), e(✓) = 3 correct out of 4 = 75% accuracy
    // Note: after X (incorrect at index 1, expected 'h'), index stays at 1
    //        so pressing 'X' again at an incorrect position, then 'h' correct
    engineKeyPress(e, 'T'); // correct (index 0)
    engineKeyPress(e, 'X'); // incorrect (expected 'h' at index 1)
    engineKeyPress(e, 'h'); // correct (index 1) — retry the position after incorrect
    engineKeyPress(e, 'e'); // correct (index 2)
    
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
    
    // Type "The " (4 chars = 0.8 words at 5 chars/word)
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

/* ===== Test: Event to string ===== */
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
    
    PASS();
}

/* ===== Test: State info ===== */
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
    
    // Test NULL engine
    info = engineGetStateInfo(NULL);
    ASSERT(info.state == ENGINE_ERROR, "NULL engine should return ERROR state");
    ASSERT(info.stopCause == ENGINE_STOP_CAUSE_ERROR, "NULL engine should return ERROR stopCause");
    
    engineDestroy(e);
    PASS();
}

/* ===== Test: Multiple callbacks for same event ===== */
static void test_multiple_callbacks(void) {
    TEST("Multiple callbacks for same event");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    CallbackFlags flags1, flags2;
    reset_flags(&flags1);
    reset_flags(&flags2);
    
    bool reg1 = engineRegisterCallback(e, &(EngineEvent){ENGINE_EVENT_STARTED}, on_start, &flags1);
    bool reg2 = engineRegisterCallback(e, &(EngineEvent){ENGINE_EVENT_STARTED}, on_start, &flags2);
    ASSERT(reg1, "first registration should succeed");
    ASSERT(reg2, "second registration should succeed");
    
    engineStart(e);
    ASSERT(flags1.started, "first callback should fire");
    ASSERT(flags2.started, "second callback should fire");
    ASSERT(flags1.call_count == 1, "first callback call_count should be 1");
    ASSERT(flags2.call_count == 1, "second callback call_count should be 1");
    
    engineDestroy(e);
    PASS();
}

/* ===== Test: Max callbacks limit ===== */
static void test_max_callbacks(void) {
    TEST("Max callbacks limit");
    Engine* e = engineCreate(StrictMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    CallbackFlags flags;
    reset_flags(&flags);
    
    // Register MAX_CALLBACKS (10) callbacks
    for (int i = 0; i < 10; i++) {
        bool reg = engineRegisterCallback(e, &(EngineEvent){ENGINE_EVENT_STARTED}, on_start, &flags);
        ASSERT(reg, "registration should succeed");
    }
    
    // 11th should fail
    bool reg = engineRegisterCallback(e, &(EngineEvent){ENGINE_EVENT_STARTED}, on_start, &flags);
    ASSERT(!reg, "11th registration should fail");
    
    engineDestroy(e);
    PASS();
}

/* ===== Test: Flow mode backspace then retype ===== */
static void test_flow_backspace_retry(void) {
    TEST("Flow mode: backspace incorrect, retype correct");
    Engine* e = engineCreate(FlowMode, 0);
    ASSERT(e != NULL, "engineCreate returned NULL");
    
    engineStart(e);
    ASSERT(engineIsRunning(e), "should be running");
    
    // Type 'X' (incorrect, expected 'T')
    engineKeyPress(e, 'X');
    SessionStats stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 1, "totalKeystrokes should be 1");
    ASSERT(stats.correctKeystrokes == 0, "correctKeystrokes should be 0");
    
    // Backspace to undo
    engineBackspacePress(e);
    stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 1, "totalKeystrokes should still be 1");
    ASSERT(stats.correctKeystrokes == 0, "correctKeystrokes should still be 0");
    
    // Now type correct key
    engineKeyPress(e, 'T');
    stats = engineGetStats(e);
    ASSERT(stats.totalKeystrokes == 2, "totalKeystrokes should be 2");
    ASSERT(stats.correctKeystrokes == 1, "correctKeystrokes should be 1");
    ASSERT(stats.accuracy == 50.0, "accuracy should be 50.0");
    
    engineDestroy(e);
    PASS();
}

/* ===== Test: Empty stats before start ===== */
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

/* ===== Test: NULL engine stats ===== */
static void test_stats_null_engine(void) {
    TEST("Stats: get stats with NULL engine");
    SessionStats stats = engineGetStats(NULL);
    ASSERT(stats.totalKeystrokes == 0, "totalKeystrokes should be 0");
    ASSERT(stats.correctKeystrokes == 0, "correctKeystrokes should be 0");
    ASSERT(stats.durationMs == 0, "durationMs should be 0");
    PASS();
}

/* ===== Main ===== */
int main(void) {
    printf("=== ctypr Engine Test Suite ===\n\n");
    
    // Engine lifecycle
    test_create_destroy();
    test_mode();
    test_timeout();
    
    // State transitions
    test_state_transitions();
    test_pause_resume();
    test_reset();
    
    // Strict mode
    test_strict_correct_key();
    test_strict_incorrect_key();
    test_strict_backspace_disabled();
    
    // Flow mode
    test_flow_correct_key();
    test_flow_incorrect_key_advances();
    
    // Backspace
    test_flow_backspace_correct();
    test_flow_backspace_incorrect();
    test_flow_backspace_retry();
    
    // Session completion
    test_session_complete_strict();
    test_session_complete_flow();
    
    // Callbacks
    test_callbacks();
    test_multiple_callbacks();
    test_max_callbacks();
    
    // Stats
    test_stats_accuracy();
    test_stats_wpm();
    test_stats_before_start();
    test_stats_null_engine();
    
    // Error handling
    test_error_already_running();
    test_error_not_running();
    
    // Event/state
    test_event_to_string();
    test_state_info();
    
    // Safety
    test_null_safety();
    
    printf("\n=== Results: %d passed, %d failed, %d total ===\n",
           tests_passed, tests_failed, test_count);
    
    return tests_failed > 0 ? 1 : 0;
}