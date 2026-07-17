#include "repository.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

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

/* Test helpers */
static char TEST_DB_PATH[256];

static void cleanupTestDb(void) {
    remove(TEST_DB_PATH);
    // Also remove journal files that SQLite might create
    remove("test_repository.db-journal");
    remove("test_repository.db-shm");
    remove("test_repository.db-wal");
}

static void setupTestDbPath(void) {
    snprintf(TEST_DB_PATH, sizeof(TEST_DB_PATH), "test_repository_%lld.db", (long long)time(NULL));
}

static SessionData createTestSession(int64_t id, const char* mode) {
    SessionData data;
    memset(&data, 0, sizeof(data));
    data.id = id;
    snprintf(data.timestamp, sizeof(data.timestamp), "%s", "2026-07-09T12:00:00+0000");
    snprintf(data.mode, sizeof(data.mode), "%s", mode);
    data.totalChars = 100;
    data.correctChars = 95;
    data.durationMs = 60000;
    data.wpm = 78.5;
    data.wpmRaw = 83.2;
    data.accuracy = 95.0;
    return data;
}

/* ===== Test: saveSession and getSession ===== */
static void test_save_and_get_session(void) {
    TEST("Repository: saveSession and getSession");
    cleanupTestDb();
    
    Repository* repo = repositoryCreate(TEST_DB_PATH);
    ASSERT(repo != NULL, "repositoryCreate returned NULL");
    
    SessionData saved = createTestSession(0, "strict");
    int64_t id = repositorySaveSession(repo, &saved);
    ASSERT(id > 0, "saveSession should return positive ID");
    
    SessionData retrieved = repositoryGetSession(repo, id);
    ASSERT(retrieved.id == id, "retrieved ID should match");
    ASSERT(strcmp(retrieved.mode, "strict") == 0, "mode should match");
    ASSERT(retrieved.totalChars == 100, "totalChars should match");
    ASSERT(retrieved.correctChars == 95, "correctChars should match");
    ASSERT(retrieved.durationMs == 60000, "durationMs should match");
    ASSERT(retrieved.wpm == 78.5, "wpm should match");
    ASSERT(retrieved.wpmRaw == 83.2, "wpmRaw should match");
    ASSERT(retrieved.accuracy == 95.0, "accuracy should match");
    
    repositoryDestroy(repo);
    cleanupTestDb();
    PASS();
}

static void test_get_session_nonexistent(void) {
    TEST("Repository: getSession returns empty for nonexistent ID");
    cleanupTestDb();
    
    Repository* repo = repositoryCreate(TEST_DB_PATH);
    ASSERT(repo != NULL, "repositoryCreate returned NULL");
    
    SessionData result = repositoryGetSession(repo, 999);
    ASSERT(result.id == 0, "nonexistent session should have id=0");
    
    repositoryDestroy(repo);
    cleanupTestDb();
    PASS();
}

/* ===== Test: getAll ===== */
static void test_get_all_sessions(void) {
    TEST("Repository: getAll returns sessions in descending order");
    cleanupTestDb();
    
    Repository* repo = repositoryCreate(TEST_DB_PATH);
    ASSERT(repo != NULL, "repositoryCreate returned NULL");
    
    SessionData s1 = createTestSession(0, "strict");
    SessionData s2 = createTestSession(0, "flow");
    SessionData s3 = createTestSession(0, "strict");
    int64_t id1 = repositorySaveSession(repo, &s1);
    int64_t id2 = repositorySaveSession(repo, &s2);
    int64_t id3 = repositorySaveSession(repo, &s3);
    
    size_t count = 0;
    SessionData* all = repositoryGetAll(repo, &count);
    ASSERT(all != NULL, "getAll should not return NULL");
    ASSERT(count == 3, "should have 3 sessions");
    ASSERT(all[0].id == id3, "first should be newest");
    ASSERT(all[1].id == id2, "second should be middle");
    ASSERT(all[2].id == id1, "third should be oldest");
    
    free(all);
    repositoryDestroy(repo);
    cleanupTestDb();
    PASS();
}

/* ===== Test: getRecent ===== */
static void test_get_recent_sessions(void) {
    TEST("Repository: getRecent returns limited results");
    cleanupTestDb();
    
    Repository* repo = repositoryCreate(TEST_DB_PATH);
    ASSERT(repo != NULL, "repositoryCreate returned NULL");
    
    for (int i = 0; i < 5; i++) {
        SessionData s = createTestSession(0, "strict");
        repositorySaveSession(repo, &s);
    }
    
    size_t count = 0;
    SessionData* recent = repositoryGetRecent(repo, 3, &count);
    ASSERT(recent != NULL, "getRecent should not return NULL");
    ASSERT(count == 3, "should return 3 sessions");
    
    free(recent);
    repositoryDestroy(repo);
    cleanupTestDb();
    PASS();
}

/* ===== Test: getCount ===== */
static void test_get_count(void) {
    TEST("Repository: getCount returns correct count");
    cleanupTestDb();
    
    Repository* repo = repositoryCreate(TEST_DB_PATH);
    ASSERT(repo != NULL, "repositoryCreate returned NULL");
    
    ASSERT(repositoryGetCount(repo) == 0, "count should be 0 initially");
    
    SessionData c1 = createTestSession(0, "strict");
    repositorySaveSession(repo, &c1);
    ASSERT(repositoryGetCount(repo) == 1, "count should be 1");
    
    SessionData c2 = createTestSession(0, "flow");
    repositorySaveSession(repo, &c2);
    ASSERT(repositoryGetCount(repo) == 2, "count should be 2");
    
    repositoryDestroy(repo);
    cleanupTestDb();
    PASS();
}

/* ===== Test: deleteSession ===== */
static void test_delete_session(void) {
    TEST("Repository: deleteSession removes session");
    cleanupTestDb();
    
    Repository* repo = repositoryCreate(TEST_DB_PATH);
    ASSERT(repo != NULL, "repositoryCreate returned NULL");
    
    SessionData del = createTestSession(0, "strict");
    int64_t id = repositorySaveSession(repo, &del);
    ASSERT(repositoryGetCount(repo) == 1, "count should be 1");
    
    bool deleted = repositoryDeleteSession(repo, id);
    ASSERT(deleted, "delete should succeed");
    ASSERT(repositoryGetCount(repo) == 0, "count should be 0 after delete");
    
    bool deletedAgain = repositoryDeleteSession(repo, id);
    ASSERT(!deletedAgain, "deleting nonexistent should return false");
    
    repositoryDestroy(repo);
    cleanupTestDb();
    PASS();
}

/* ===== Test: getSessionsByMode ===== */
static void test_get_sessions_by_mode(void) {
    TEST("Repository: getSessionsByMode filters correctly");
    cleanupTestDb();
    
    Repository* repo = repositoryCreate(TEST_DB_PATH);
    ASSERT(repo != NULL, "repositoryCreate returned NULL");
    
    SessionData m1 = createTestSession(0, "strict");
    SessionData m2 = createTestSession(0, "flow");
    SessionData m3 = createTestSession(0, "strict");
    SessionData m4 = createTestSession(0, "flow");
    repositorySaveSession(repo, &m1);
    repositorySaveSession(repo, &m2);
    repositorySaveSession(repo, &m3);
    repositorySaveSession(repo, &m4);
    
    size_t count = 0;
    SessionData* strict = repositoryGetAll(repo, &count);
    ASSERT(strict != NULL, "getAll should not return NULL");
    
    // Count strict modes manually
    size_t strictCount = 0;
    for (size_t i = 0; i < count; i++) {
        if (strcmp(strict[i].mode, "strict") == 0) strictCount++;
    }
    ASSERT(strictCount == 2, "should have 2 strict sessions");
    
    free(strict);
    repositoryDestroy(repo);
    cleanupTestDb();
    PASS();
}

/* ===== Test: getBestWpm ===== */
static void test_get_best_wpm(void) {
    TEST("Repository: getBestWpm returns highest WPM session");
    cleanupTestDb();
    
    Repository* repo = repositoryCreate(TEST_DB_PATH);
    ASSERT(repo != NULL, "repositoryCreate returned NULL");
    
    SessionData s1 = createTestSession(0, "strict");
    s1.wpm = 60.0;
    repositorySaveSession(repo, &s1);
    
    SessionData s2 = createTestSession(0, "flow");
    s2.wpm = 90.0;
    repositorySaveSession(repo, &s2);
    
    SessionData best = repositoryGetBestWpm(repo);
    ASSERT(best.wpm == 90.0, "best WPM should be 90.0");
    
    repositoryDestroy(repo);
    cleanupTestDb();
    PASS();
}

/* ===== Test: getBestRawWpm ===== */
static void test_get_best_raw_wpm(void) {
    TEST("Repository: getBestRawWpm returns highest raw WPM session");
    cleanupTestDb();
    
    Repository* repo = repositoryCreate(TEST_DB_PATH);
    ASSERT(repo != NULL, "repositoryCreate returned NULL");
    
    SessionData s1 = createTestSession(0, "strict");
    s1.wpmRaw = 70.0;
    repositorySaveSession(repo, &s1);
    
    SessionData s2 = createTestSession(0, "flow");
    s2.wpmRaw = 95.0;
    repositorySaveSession(repo, &s2);
    
    SessionData best = repositoryGetBestRawWpm(repo);
    ASSERT(best.wpmRaw == 95.0, "best raw WPM should be 95.0");
    
    repositoryDestroy(repo);
    cleanupTestDb();
    PASS();
}

/* ===== Test: getAverageWpm ===== */
static void test_get_average_wpm(void) {
    TEST("Repository: getAverageWpm calculates correctly");
    cleanupTestDb();
    
    Repository* repo = repositoryCreate(TEST_DB_PATH);
    ASSERT(repo != NULL, "repositoryCreate returned NULL");
    
    ASSERT(repositoryGetAverageWpm(repo) == 0.0, "avg should be 0.0 for empty DB");
    
    SessionData avg1 = createTestSession(0, "strict");
    avg1.wpm = 60.0;
    repositorySaveSession(repo, &avg1);
    
    SessionData avg2 = createTestSession(0, "flow");
    avg2.wpm = 80.0;
    repositorySaveSession(repo, &avg2);
    
    double avg = repositoryGetAverageWpm(repo);
    ASSERT(avg == 70.0, "avg should be 70.0");
    
    repositoryDestroy(repo);
    cleanupTestDb();
    PASS();
}

/* ===== Test: clearAll ===== */
static void test_clear_all(void) {
    TEST("Repository: clearAll removes all sessions");
    cleanupTestDb();
    
    Repository* repo = repositoryCreate(TEST_DB_PATH);
    ASSERT(repo != NULL, "repositoryCreate returned NULL");
    
    SessionData c1 = createTestSession(0, "strict");
    SessionData c2 = createTestSession(0, "flow");
    SessionData c3 = createTestSession(0, "strict");
    repositorySaveSession(repo, &c1);
    repositorySaveSession(repo, &c2);
    repositorySaveSession(repo, &c3);
    
    ASSERT(repositoryGetCount(repo) == 3, "count should be 3");
    
    repositoryClearAll(repo);
    ASSERT(repositoryGetCount(repo) == 0, "count should be 0 after clear");
    
    repositoryDestroy(repo);
    cleanupTestDb();
    PASS();
}

/* ===== Test: NULL safety ===== */
static void test_repository_null_safety(void) {
    TEST("Repository: NULL safety");
    
    repositorySaveSession(NULL, NULL);
    repositoryGetSession(NULL, 0);
    
    size_t count = 0;
    repositoryGetAll(NULL, &count);
    repositoryGetRecent(NULL, 5, &count);
    repositoryGetCount(NULL);
    repositoryDeleteSession(NULL, 1);
    repositoryClearAll(NULL);
    repositoryGetBestWpm(NULL);
    repositoryGetBestRawWpm(NULL);
    repositoryGetAverageWpm(NULL);
    repositoryDestroy(NULL);
    
    PASS();
}

/* ===== Main ===== */
int main(void) {
    printf("=== ctypr Repository Test Suite ===\n\n");
    
    setupTestDbPath();
    test_save_and_get_session();
    test_get_session_nonexistent();
    test_get_all_sessions();
    test_get_recent_sessions();
    test_get_count();
    test_delete_session();
    test_get_sessions_by_mode();
    test_get_best_wpm();
    test_get_best_raw_wpm();
    test_get_average_wpm();
    test_clear_all();
    test_repository_null_safety();
    
    printf("\n=== Results: %d passed, %d failed, %d total ===\n",
           tests_passed, tests_failed, test_count);
    
    return tests_failed > 0 ? 1 : 0;
}