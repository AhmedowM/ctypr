#include "engine.h"
#include "callback.h"
#include "error.h"
#include "event.h"
#include "state.h"
#include "stats.h"
#include "content.h"
#include "formatter.h"
#include "repository.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Callback handlers ───────────────────────────────────────────────────── */

static void on_event(Engine* e, void* data) {
    (void)e;
    printf("  [EVENT] %s\n", (const char*)data);
}

/* ── Main demo ───────────────────────────────────────────────────────────── */

int main(void) {
    printf("=== ctypr Library Usage Demo ===\n\n");

    /* 1. Logger */
    printf("── 1. Logger ───────────────────────────────────────────\n");
    Logger* log = loggerCreate(LOG_LEVEL_INFO, true);
    loggerLog(log, LOG_LEVEL_INFO, "Logger created");

    /* 2. Repository */
    printf("\n── 2. Repository ───────────────────────────────────────\n");
    Repository* repo = repositoryCreate("typr_demo.db");
    if (!repo) {
        fprintf(stderr, "Failed to create repository\n");
        loggerDestroy(log);
        return 1;
    }
    repositorySetLogger(repo, log);
    loggerLog(log, LOG_LEVEL_INFO, "Repository opened");

    /* 3. Content Provider (string example) */
    printf("\n── 3. Content Provider ─────────────────────────────────\n");
    ContentProvider* content = contentProviderFromString(
        "The quick brown fox jumps over the lazy dog. "
        "Pack my box with five dozen liquor jugs. "
        "How vexingly quick daft zebras jump."
    );
    if (!content) {
        fprintf(stderr, "Failed to create content provider\n");
        repositoryDestroy(repo);
        loggerDestroy(log);
        return 1;
    }
    contentProviderSetLogger(content, log);
    ContentChunk rawContent = contentProviderGetNext(content);
    printf("  Content (%zu chars): %.60s...\n", rawContent.length, rawContent.text);

    /* 4. Formatter */
    printf("\n── 4. Formatter ────────────────────────────────────────\n");
    Formatter* formatter = formatterCreate();
    if (!formatter) {
        fprintf(stderr, "Failed to create formatter\n");
        contentProviderDestroy(content);
        repositoryDestroy(repo);
        loggerDestroy(log);
        return 1;
    }
    formatterSetLogger(formatter, log);
    ContentChunk chunk = formatterFormat(formatter, rawContent.text, 40);
    printf("  First chunk (%zu chars): \"%s\"\n", chunk.length, chunk.text);

    /* 5. Engine */
    printf("\n── 5. Engine ───────────────────────────────────────────\n");
    EngineConfig engineConfig = {
        .mode = FlowMode,
        .timeout = 30,
        .contentProvider = content
    };
    Engine* engine = engineCreate(&engineConfig);
    if (!engine) {
        fprintf(stderr, "Failed to create engine\n");
        formatterDestroy(formatter);
        contentProviderDestroy(content);
        repositoryDestroy(repo);
        loggerDestroy(log);
        return 1;
    }
    engineSetLogger(engine, log);
    engineOnStarted(engine, on_event, "STARTED");
    engineOnStopped(engine, on_event, "STOPPED");
    engineOnTimeout(engine, on_event, "TIMEOUT");
    engineOnFinished(engine, on_event, "FINISHED");
    engineOnCorrectKeystroke(engine, on_event, "CORRECT_KEYSTROKE");
    engineOnIncorrectKeystroke(engine, on_event, "INCORRECT_KEYSTROKE");
    engineOnBackspace(engine, on_event, "BACKSPACE");
    engineOnPaused(engine, on_event, "PAUSED");
    engineOnResumed(engine, on_event, "RESUMED");

    /* 6. Run a session */
    printf("\n── 6. Typing Session ───────────────────────────────────\n");
    engineStart(engine);
    printf("  State: %s\n", engineIsRunning(engine) ? "RUNNING" : "?");
    printf("  Text:  \"%s\"\n", rawContent.text);

    for (const char* p = rawContent.text; *p; p++) {
        engineKeyPress(engine, *p);
        if (engineIsCompleted(engine) || engineIsTimedOut(engine)) break;
    }

    /* 7. Stats */
    printf("\n── 7. Statistics ───────────────────────────────────────\n");
    SessionStats stats = engineGetStats(engine);
    printf("  Total keystrokes:  %u\n", stats.totalKeystrokes);
    printf("  Correct keystrokes: %u\n", stats.correctKeystrokes);
    printf("  Accuracy:          %.1f%%\n", stats.accuracy);
    printf("  Duration:          %lld ms\n", (long long)stats.durationMs);
    printf("  Raw WPM:           %.1f\n", stats.wpmRaw);
    printf("  Adjusted WPM:      %.1f\n", stats.wpm);
    printf("  Completed:         %s\n", engineIsCompleted(engine) ? "yes" : "no");

    /* 8. Save session to repository */
    printf("\n── 8. Persistence ──────────────────────────────────────\n");
    SessionData data;
    memset(&data, 0, sizeof(data));
    strcpy(data.mode, "flow");
    data.totalChars   = stats.totalKeystrokes;
    data.correctChars = stats.correctKeystrokes;
    data.durationMs   = stats.durationMs;
    data.wpm          = stats.wpm;
    data.wpmRaw       = stats.wpmRaw;
    data.accuracy     = stats.accuracy;

    int64_t id = repositorySaveSession(repo, &data);
    if (id >= 0) {
        printf("  Session saved with ID: %lld\n", (long long)id);
    } else {
        printf("  Failed to save session\n");
    }

    /* 9. Query repository */
    printf("\n── 9. Session History ──────────────────────────────────\n");
    size_t count;
    SessionData* sessions = repositoryGetAll(repo, &count);
    printf("  Total sessions: %zu\n", count);
    for (size_t i = 0; i < count; i++) {
        printf("    #%lld: %s | %.1f WPM | %.1f%% acc\n",
               (long long)sessions[i].id,
               sessions[i].mode,
               sessions[i].wpm,
               sessions[i].accuracy);
    }
    free(sessions);

    int64_t totalSessions = repositoryGetCount(repo);
    double avgWpm = repositoryGetAverageWpm(repo);
    printf("  Average WPM across %lld sessions: %.1f\n",
           (long long)totalSessions, avgWpm);

    /* 10. State info */
    printf("\n── 10. State Info ──────────────────────────────────────\n");
    EngineStateInfo info = engineGetStateInfo(engine);
    printf("  Engine state: %s\n",
           info.state == ENGINE_IDLE ? "IDLE" :
           info.state == ENGINE_RUNNING ? "RUNNING" :
           info.state == ENGINE_PAUSED ? "PAUSED" : "ERROR");
    printf("  Stop cause:   %s\n",
           info.stopCause == ENGINE_STOP_CAUSE_NONE ? "NONE" :
           info.stopCause == ENGINE_STOP_CAUSE_TIMEOUT ? "TIMEOUT" :
           info.stopCause == ENGINE_STOP_CAUSE_FINISHED ? "FINISHED" :
           info.stopCause == ENGINE_STOP_CAUSE_USER ? "USER" :
           info.stopCause == ENGINE_STOP_CAUSE_ERROR ? "ERROR" : "UNKNOWN");

    /* 11. Database content provider */
    printf("\n── 11. Database Content Provider ───────────────────────\n");
    {
        /* The DB file must already exist with tables:
         *   common_words(id, word, word_length, frequency_rank)
         *   random_words(id, word, word_length, difficulty_rating)
         *   typing_sentences(id, text_content, char_count, word_count, ...)
         * A missing DB returns empty content.
         *
         * See src/content/content.h for ContentMode and configuration. */
        ContentProvider* dbContent = contentProviderFromDatabase("nonexistent.db");
        contentProviderSetLogger(dbContent, log);
        contentProviderSetMode(dbContent, CONTENT_MODE_COMMON_WORDS);
        contentProviderSetContentLimit(dbContent, 50);
        ContentChunk dbChunk = contentProviderGetNext(dbContent);
        printf("  DB content length: %zu (0 = file missing/empty)\n", dbChunk.length);
        contentProviderDestroy(dbContent);
    }

    /* 12. Event/error string conversion */
    printf("\n── 12. String Conversions ──────────────────────────────\n");
    {
        char buf[64];
        engineEventToString(ENGINE_EVENT_STARTED, buf, sizeof(buf));
        printf("  Event: %s\n", buf);
        engineErrorToString(ENGINE_ERROR_ALREADY_RUNNING, buf, sizeof(buf));
        printf("  Error: %s\n", buf);
    }

    /* Cleanup */
    printf("\n── Cleanup ────────────────────────────────────────────\n");
    engineDestroy(engine);
    formatterDestroy(formatter);
    contentProviderDestroy(content);
    repositoryDestroy(repo);
    loggerDestroy(log);
    remove("typr_demo.db");

    printf("\n=== Demo complete ===\n");
    return 0;
}
