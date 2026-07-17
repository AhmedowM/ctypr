#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* Helper: write all log messages to a file, read it back, remove it */
static char* capture_file_log(Logger* logger, LogLevel level, const char* msg) {
    static char buf[512];
    const char* tmpname = "test_capture.txt";
    bool ok = loggerAddFile(logger, tmpname);
    if (!ok) return NULL;
    loggerLog(logger, level, msg);
    loggerDestroy(logger);

    FILE* f = fopen(tmpname, "r");
    if (!f) return NULL;
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';
    fclose(f);
    remove(tmpname);
    return buf;
}

static void test_create_destroy(void) {
    TEST("Logger: create and destroy");
    Logger* logger = loggerCreate(LOG_LEVEL_INFO, false);
    ASSERT(logger != NULL, "loggerCreate returned NULL");
    loggerDestroy(logger);
    PASS();
}

static void test_default_level(void) {
    TEST("Logger: level defaults correctly");
    Logger* logger = loggerCreate(LOG_LEVEL_WARNING, false);
    ASSERT(logger != NULL, "loggerCreate returned NULL");
    ASSERT(loggerGetLevel(logger) == LOG_LEVEL_WARNING, "level should be WARNING");
    loggerDestroy(logger);
    PASS();
}

static void test_set_get_level(void) {
    TEST("Logger: set and get level");
    Logger* logger = loggerCreate(LOG_LEVEL_DEBUG, false);
    ASSERT(logger != NULL, "loggerCreate returned NULL");

    loggerSetLevel(logger, LOG_LEVEL_ERROR);
    ASSERT(loggerGetLevel(logger) == LOG_LEVEL_ERROR, "level should be ERROR");

    loggerSetLevel(logger, LOG_LEVEL_DEBUG);
    ASSERT(loggerGetLevel(logger) == LOG_LEVEL_DEBUG, "level should be DEBUG");

    loggerDestroy(logger);
    PASS();
}

static void test_log_to_stdout(void) {
    TEST("Logger: log to stdout does not crash");
    Logger* logger = loggerCreate(LOG_LEVEL_DEBUG, true);
    ASSERT(logger != NULL, "loggerCreate returned NULL");
    loggerLog(logger, LOG_LEVEL_INFO, "stdout smoke test");
    loggerDestroy(logger);
    PASS();
}

static void test_log_to_file(void) {
    TEST("Logger: log to file");
    Logger* logger = loggerCreate(LOG_LEVEL_DEBUG, false);
    ASSERT(logger != NULL, "loggerCreate returned NULL");

    bool ok = loggerAddFile(logger, "test_log.txt");
    ASSERT(ok, "loggerAddFile should succeed");

    loggerLog(logger, LOG_LEVEL_WARNING, "file test message");
    loggerDestroy(logger);

    FILE* f = fopen("test_log.txt", "r");
    ASSERT(f != NULL, "log file should exist");
    char buf[256] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';
    fclose(f);
    remove("test_log.txt");

    ASSERT(strstr(buf, "[WARNING]") != NULL, "output should contain [WARNING]");
    ASSERT(strstr(buf, "file test message") != NULL, "output should contain message");

    PASS();
}

static void test_level_filtering(void) {
    TEST("Logger: messages below level are suppressed via file");
    Logger* logger = loggerCreate(LOG_LEVEL_WARNING, false);
    ASSERT(logger != NULL, "loggerCreate returned NULL");

    bool ok = loggerAddFile(logger, "test_filter.txt");
    ASSERT(ok, "add file");

    loggerLog(logger, LOG_LEVEL_DEBUG, "debug msg");
    loggerLog(logger, LOG_LEVEL_INFO, "info msg");
    loggerLog(logger, LOG_LEVEL_WARNING, "warning msg");
    loggerLog(logger, LOG_LEVEL_ERROR, "error msg");
    loggerDestroy(logger);

    FILE* f = fopen("test_filter.txt", "r");
    ASSERT(f != NULL, "log file should exist");
    char buf[512] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';
    fclose(f);
    remove("test_filter.txt");

    ASSERT(strstr(buf, "debug msg") == NULL, "DEBUG should be suppressed");
    ASSERT(strstr(buf, "info msg") == NULL, "INFO should be suppressed");
    ASSERT(strstr(buf, "warning msg") != NULL, "WARNING should appear");
    ASSERT(strstr(buf, "error msg") != NULL, "ERROR should appear");

    PASS();
}

static void test_toggle_stdout(void) {
    TEST("Logger: toggle stdout on/off does not crash");
    Logger* logger = loggerCreate(LOG_LEVEL_DEBUG, true);
    ASSERT(logger != NULL, "loggerCreate returned NULL");

    loggerLogToStdout(logger, false);
    loggerLog(logger, LOG_LEVEL_INFO, "should not appear on stdout");

    loggerLogToStdout(logger, true);
    loggerLog(logger, LOG_LEVEL_INFO, "should appear on stdout");

    loggerDestroy(logger);
    PASS();
}

static void test_add_multiple_files(void) {
    TEST("Logger: add multiple files");
    Logger* logger = loggerCreate(LOG_LEVEL_DEBUG, false);
    ASSERT(logger != NULL, "loggerCreate returned NULL");

    ASSERT(loggerAddFile(logger, "test_log1.txt"), "add file 1");
    ASSERT(loggerAddFile(logger, "test_log2.txt"), "add file 2");

    loggerLog(logger, LOG_LEVEL_INFO, "multi-file test");
    loggerDestroy(logger);

    for (int i = 1; i <= 2; i++) {
        char name[32];
        snprintf(name, sizeof(name), "test_log%d.txt", i);
        FILE* f = fopen(name, "r");
        ASSERT(f != NULL, "log file should exist");
        char buf[128] = {0};
        fread(buf, 1, sizeof(buf) - 1, f);
        fclose(f);
        remove(name);
        ASSERT(strstr(buf, "multi-file test") != NULL, "file should contain message");
    }

    PASS();
}

static void test_max_files_limit(void) {
    TEST("Logger: max files limit enforced");
    Logger* logger = loggerCreate(LOG_LEVEL_DEBUG, false);
    ASSERT(logger != NULL, "loggerCreate returned NULL");

    ASSERT(loggerAddFile(logger, "test_mf1.txt"), "add 1");
    ASSERT(loggerAddFile(logger, "test_mf2.txt"), "add 2");
    ASSERT(loggerAddFile(logger, "test_mf3.txt"), "add 3");
    ASSERT(loggerAddFile(logger, "test_mf4.txt"), "add 4");

    ASSERT(!loggerAddFile(logger, "test_mf5.txt"), "5th add should fail");

    loggerDestroy(logger);
    remove("test_mf1.txt");
    remove("test_mf2.txt");
    remove("test_mf3.txt");
    remove("test_mf4.txt");
    PASS();
}

static void test_log_output_format(void) {
    TEST("Logger: output format is [LEVEL] message");
    Logger* logger = loggerCreate(LOG_LEVEL_DEBUG, false);
    ASSERT(logger != NULL, "loggerCreate returned NULL");

    bool ok = loggerAddFile(logger, "test_fmt.txt");
    ASSERT(ok, "add file");

    loggerLog(logger, LOG_LEVEL_ERROR, "error!");
    loggerDestroy(logger);

    FILE* f = fopen("test_fmt.txt", "r");
    ASSERT(f != NULL, "log file should exist");
    char buf[256] = {0};
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';
    fclose(f);
    remove("test_fmt.txt");

    ASSERT(strcmp(buf, "[ERROR] error!\n") == 0, "format should match exactly");

    PASS();
}

static void test_output_format_all_levels(void) {
    TEST("Logger: output format for all levels");
    const char* levels[] = {"DEBUG", "INFO", "WARNING", "ERROR"};
    LogLevel lvls[] = {LOG_LEVEL_DEBUG, LOG_LEVEL_INFO, LOG_LEVEL_WARNING, LOG_LEVEL_ERROR};

    for (int i = 0; i < 4; i++) {
        Logger* logger = loggerCreate(LOG_LEVEL_DEBUG, false);
        ASSERT(logger != NULL, "loggerCreate returned NULL");

        char fname[32];
        snprintf(fname, sizeof(fname), "test_lvl_%d.txt", i);
        ASSERT(loggerAddFile(logger, fname), "add file");

        loggerLog(logger, lvls[i], "msg");
        loggerDestroy(logger);

        FILE* f = fopen(fname, "r");
        ASSERT(f != NULL, "log file should exist");
        char expected[64];
        snprintf(expected, sizeof(expected), "[%s] msg\n", levels[i]);
        char buf[64] = {0};
        fread(buf, 1, sizeof(buf) - 1, f);
        fclose(f);
        remove(fname);

        ASSERT(strcmp(buf, expected) == 0, "format should match");
    }

    PASS();
}

static void test_log_level_none_suppresses_all(void) {
    TEST("Logger: LOG_LEVEL_NONE suppresses all messages");
    Logger* logger = loggerCreate(LOG_LEVEL_NONE, false);
    ASSERT(logger != NULL, "loggerCreate returned NULL");

    bool ok = loggerAddFile(logger, "test_none.txt");
    ASSERT(ok, "add file");

    loggerLog(logger, LOG_LEVEL_DEBUG, "debug");
    loggerLog(logger, LOG_LEVEL_INFO, "info");
    loggerLog(logger, LOG_LEVEL_WARNING, "warning");
    loggerLog(logger, LOG_LEVEL_ERROR, "error");
    loggerDestroy(logger);

    FILE* f = fopen("test_none.txt", "r");
    ASSERT(f != NULL, "log file should exist");
    char buf[256] = {0};
    fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    remove("test_none.txt");

    ASSERT(buf[0] == '\0', "all messages should be suppressed when level is NONE");

    PASS();
}

static void test_stdout_enabled_with_file(void) {
    TEST("Logger: stdout + file both receive messages");
    Logger* logger = loggerCreate(LOG_LEVEL_DEBUG, true);
    ASSERT(logger != NULL, "loggerCreate returned NULL");

    ASSERT(loggerAddFile(logger, "test_dual.txt"), "add file");

    loggerLog(logger, LOG_LEVEL_INFO, "dual output");
    loggerDestroy(logger);

    FILE* f = fopen("test_dual.txt", "r");
    ASSERT(f != NULL, "log file should exist");
    char buf[128] = {0};
    fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    remove("test_dual.txt");

    ASSERT(strstr(buf, "[INFO]") != NULL, "file should receive output");
    ASSERT(strstr(buf, "dual output") != NULL, "file should contain message");

    PASS();
}

static void test_logger_add_file_failure(void) {
    TEST("Logger: addFile returns false on invalid path");
    Logger* logger = loggerCreate(LOG_LEVEL_DEBUG, false);
    ASSERT(logger != NULL, "loggerCreate returned NULL");

#ifdef _WIN32
    bool ok = loggerAddFile(logger, "\\\\invalid_path\\test.log");
#else
    bool ok = loggerAddFile(logger, "/nonexistent_dir_xyz/test.log");
#endif
    ASSERT(!ok, "addFile should fail for invalid path");

    loggerDestroy(logger);
    PASS();
}

static void test_null_safety(void) {
    TEST("Logger: NULL safety");
    loggerDestroy(NULL);
    loggerSetLevel(NULL, LOG_LEVEL_INFO);
    loggerGetLevel(NULL);
    loggerLogToStdout(NULL, false);
    ASSERT(!loggerAddFile(NULL, "test.log"), "addFile with NULL should return false");
    loggerLog(NULL, LOG_LEVEL_INFO, "test");
    PASS();
}

int main(void) {
    printf("=== ctypr Logger Test Suite ===\n\n");

    test_create_destroy();
    test_default_level();
    test_set_get_level();
    test_log_to_stdout();
    test_log_to_file();
    test_level_filtering();
    test_toggle_stdout();
    test_add_multiple_files();
    test_max_files_limit();
    test_log_output_format();
    test_output_format_all_levels();
    test_log_level_none_suppresses_all();
    test_stdout_enabled_with_file();
    test_logger_add_file_failure();
    test_null_safety();

    printf("\n=== Results: %d passed, %d failed, %d total ===\n",
           tests_passed, tests_failed, test_count);

    return tests_failed > 0 ? 1 : 0;
}
