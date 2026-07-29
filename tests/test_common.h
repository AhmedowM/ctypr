#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#define SLEEP_MS(ms) Sleep(ms)
#else
#include <unistd.h>
#define SLEEP_MS(ms) usleep((ms) * 1000)
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

#define TEST_SUMMARY() \
    printf("\n=== Results: %d passed, %d failed, %d total ===\n", \
           tests_passed, tests_failed, test_count); \
    return tests_failed > 0 ? 1 : 0

#ifdef _WIN32
static void buildTestDbPath(char *buf, size_t size, const char *prefix) {
    snprintf(buf, size, "%s_%lld_%lld.db", prefix, (long long)time(NULL), (long long)GetTickCount64());
}
#else
static void buildTestDbPath(char *buf, size_t size, const char *prefix) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    snprintf(buf, size, "%s_%lld_%ld.db", prefix, (long long)ts.tv_sec, (long)ts.tv_nsec);
}
#endif

/* Declare TEST_DB_PATH, cleanupTestDb(), setupTestDbPath() with a prefix string.
   Example: TEST_DB_HELPERS("test_repository") */
#define TEST_DB_HELPERS(prefix) \
static char TEST_DB_PATH[256]; \
\
static void cleanupTestDb(void) { \
    remove(TEST_DB_PATH); \
    remove(prefix ".db-journal"); \
    remove(prefix ".db-shm"); \
    remove(prefix ".db-wal"); \
} \
\
static void setupTestDbPath(void) { \
    buildTestDbPath(TEST_DB_PATH, sizeof(TEST_DB_PATH), prefix); \
}

#endif
