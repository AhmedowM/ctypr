#include "repository.h"
#include "logger.h"
#include "platform_internal.h"
#include "test_common.h"

TEST_DB_HELPERS("test_concurrency")

static SessionData makeSession(const char* mode, double wpm) {
    SessionData data;
    memset(&data, 0, sizeof(data));
    snprintf(data.timestamp, sizeof(data.timestamp), "2026-07-09T12:00:00");
    snprintf(data.mode, sizeof(data.mode), "%s", mode);
    data.totalChars = 100;
    data.correctChars = 95;
    data.durationMs = 60000;
    data.wpm = wpm;
    data.wpmRaw = wpm + 5.0;
    data.accuracy = 95.0;
    return data;
}

typedef struct {
    Repository* repo;
    int thread_id;
    int iterations;
    int success_count;
} RepoThreadJob;

typedef struct {
    Logger* logger;
    int thread_id;
    int iterations;
} LoggerThreadJob;

/* ── Test 1: Concurrent saves ────────────────────────────────── */

static THREAD_FUNC(concurrent_save_thread) {
    RepoThreadJob* job = (RepoThreadJob*)arg;
    job->success_count = 0;
    for (int i = 0; i < job->iterations; i++) {
        SessionData s = makeSession(
            (i % 2 == 0) ? "strict" : "flow",
            60.0 + (double)(job->thread_id * 10 + i)
        );
        int64_t id = repositorySaveSession(job->repo, &s);
        if (id > 0) job->success_count++;
        if (i % 10 == 0) SLEEP_MS(1);
    }
    return 0;
}

static void test_concurrent_saves(void) {
    TEST("Repository: concurrent saves from 4 threads");
    cleanupTestDb();

    Repository* repo = repositoryCreate(TEST_DB_PATH);
    ASSERT(repo != NULL, "repositoryCreate returned NULL");

#define NUM_SAVE_THREADS 4
#define SAVES_PER_THREAD 50

    ctypr_thread_t threads[NUM_SAVE_THREADS];
    RepoThreadJob jobs[NUM_SAVE_THREADS];

    for (int i = 0; i < NUM_SAVE_THREADS; i++) {
        jobs[i].repo = repo;
        jobs[i].thread_id = i;
        jobs[i].iterations = SAVES_PER_THREAD;
        THREAD_CREATE(&threads[i], concurrent_save_thread, &jobs[i]);
    }

    int total_success = 0;
    for (int i = 0; i < NUM_SAVE_THREADS; i++) {
        THREAD_JOIN(threads[i]);
        total_success += jobs[i].success_count;
    }

    int64_t expected = NUM_SAVE_THREADS * SAVES_PER_THREAD;
    ASSERT(total_success == expected, "all saves should succeed");

    int64_t db_count = repositoryGetCount(repo);
    ASSERT(db_count == expected, "DB count should match total saves");

    size_t all_count = 0;
    SessionData* all = repositoryGetAll(repo, &all_count);
    ASSERT(all != NULL, "getAll should succeed");
    ASSERT(all_count == (size_t)db_count, "getAll count should match getCount");
    for (size_t i = 0; i < all_count; i++) {
        ASSERT(all[i].id > 0, "each session should have positive ID");
        ASSERT(all[i].wpm > 0.0, "each session should have positive WPM");
    }
    free(all);

    repositoryDestroy(repo);
    cleanupTestDb();
    PASS();
}

/* ── Test 2: Concurrent reads and writes ──────────────────────── */

static THREAD_FUNC(concurrent_reader_thread) {
    RepoThreadJob* job = (RepoThreadJob*)arg;
    job->success_count = 0;
    for (int i = 0; i < job->iterations; i++) {
        size_t count = 0;
        SessionData* all = repositoryGetAll(job->repo, &count);
        if (all) {
            for (size_t j = 0; j < count; j++) {
                if (all[j].id > 0) job->success_count++;
            }
            free(all);
        }
        repositoryGetCount(job->repo);
        repositoryGetBestWpm(job->repo);
        repositoryGetBestRawWpm(job->repo);
        repositoryGetAverageWpm(job->repo);
        if (i % 10 == 0) SLEEP_MS(1);
    }
    return 0;
}

static void test_concurrent_read_write(void) {
    TEST("Repository: concurrent reads + writes (2 writers, 2 readers)");
    cleanupTestDb();

    Repository* repo = repositoryCreate(TEST_DB_PATH);
    ASSERT(repo != NULL, "repositoryCreate returned NULL");

#define NUM_RW_THREADS 4
#define RW_ITERATIONS 40

    ctypr_thread_t threads[NUM_RW_THREADS];
    RepoThreadJob jobs[NUM_RW_THREADS];

    for (int i = 0; i < NUM_RW_THREADS; i++) {
        jobs[i].repo = repo;
        jobs[i].thread_id = i;
        jobs[i].iterations = RW_ITERATIONS;
        THREAD_CREATE(&threads[i],
            (i < 2) ? concurrent_save_thread : concurrent_reader_thread,
            &jobs[i]);
    }

    for (int i = 0; i < NUM_RW_THREADS; i++) {
        THREAD_JOIN(threads[i]);
    }

    ASSERT(jobs[0].success_count == RW_ITERATIONS, "writer 0 should succeed all saves");
    ASSERT(jobs[1].success_count == RW_ITERATIONS, "writer 1 should succeed all saves");
    ASSERT(jobs[2].success_count > 0, "reader 0 should have processed sessions");
    ASSERT(jobs[3].success_count > 0, "reader 1 should have processed sessions");

    int64_t expected = 2 * RW_ITERATIONS;
    ASSERT(repositoryGetCount(repo) == expected, "final count should match total saves");

    repositoryDestroy(repo);
    cleanupTestDb();
    PASS();
}

/* ── Test 3: Concurrent logger ────────────────────────────────── */

static THREAD_FUNC(concurrent_log_thread) {
    LoggerThreadJob* job = (LoggerThreadJob*)arg;
    char buf[64];
    for (int i = 0; i < job->iterations; i++) {
        snprintf(buf, sizeof(buf), "thread %d message %d", job->thread_id, i);
        loggerLog(job->logger, LOG_LEVEL_INFO, buf);
        if (i % 20 == 0) SLEEP_MS(1);
    }
    return 0;
}

static void test_concurrent_logger(void) {
    TEST("Logger: concurrent logging from 4 threads to file");

    const char* logfile = "test_concurrent_log.txt";
    remove(logfile);

    Logger* logger = loggerCreate(LOG_LEVEL_DEBUG, false);
    ASSERT(logger != NULL, "loggerCreate returned NULL");
    ASSERT(loggerAddFile(logger, logfile), "loggerAddFile should succeed");

#define NUM_LOG_THREADS 4
#define LOGS_PER_THREAD 100

    ctypr_thread_t threads[NUM_LOG_THREADS];
    LoggerThreadJob jobs[NUM_LOG_THREADS];

    for (int i = 0; i < NUM_LOG_THREADS; i++) {
        jobs[i].logger = logger;
        jobs[i].thread_id = i;
        jobs[i].iterations = LOGS_PER_THREAD;
        THREAD_CREATE(&threads[i], concurrent_log_thread, &jobs[i]);
    }

    for (int i = 0; i < NUM_LOG_THREADS; i++) {
        THREAD_JOIN(threads[i]);
    }

    loggerDestroy(logger);

    FILE* f = fopen(logfile, "r");
    ASSERT(f != NULL, "log file should exist");

    int line_count = 0;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        line_count++;
        ASSERT(line[0] == '[', "each log line should start with '['");
    }
    fclose(f);

    ASSERT(line_count == NUM_LOG_THREADS * LOGS_PER_THREAD,
           "log file should have all messages");

    remove(logfile);
    PASS();
}

/* ── Test 4: Logger concurrent log + reconfigure ──────────────── */

static THREAD_FUNC(reconfigure_thread) {
    Logger* logger = ((LoggerThreadJob*)arg)->logger;
    for (int i = 0; i < 20; i++) {
        loggerLog(logger, LOG_LEVEL_DEBUG, "reconfigure: debug");
        loggerLogToStdout(logger, (i % 3 == 0));
        loggerSetLevel(logger,
            (i % 4 == 0) ? LOG_LEVEL_DEBUG :
            (i % 4 == 1) ? LOG_LEVEL_INFO :
            (i % 4 == 2) ? LOG_LEVEL_WARNING : LOG_LEVEL_ERROR);
        SLEEP_MS(2);
    }
    return 0;
}

static void test_concurrent_logger_reconfigure(void) {
    TEST("Logger: concurrent logging + level reconfigure");

    const char* logfile = "test_concurrent_reconfig.txt";
    remove(logfile);

    Logger* logger = loggerCreate(LOG_LEVEL_DEBUG, false);
    ASSERT(logger != NULL, "loggerCreate returned NULL");
    ASSERT(loggerAddFile(logger, logfile), "loggerAddFile should succeed");

#define NUM_RECONF_LOG 3
#define RECONF_LOGS 60

    ctypr_thread_t log_threads[NUM_RECONF_LOG];
    LoggerThreadJob log_jobs[NUM_RECONF_LOG];

    for (int i = 0; i < NUM_RECONF_LOG; i++) {
        log_jobs[i].logger = logger;
        log_jobs[i].thread_id = i;
        log_jobs[i].iterations = RECONF_LOGS;
        THREAD_CREATE(&log_threads[i], concurrent_log_thread, &log_jobs[i]);
    }

    LoggerThreadJob reconf_job;
    reconf_job.logger = logger;
    ctypr_thread_t reconf_thread;
    THREAD_CREATE(&reconf_thread, reconfigure_thread, &reconf_job);

    for (int i = 0; i < NUM_RECONF_LOG; i++) {
        THREAD_JOIN(log_threads[i]);
    }
    THREAD_JOIN(reconf_thread);

    loggerDestroy(logger);

    FILE* f = fopen(logfile, "r");
    ASSERT(f != NULL, "log file should exist");

    int line_count = 0;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        line_count++;
    }
    fclose(f);

    ASSERT(line_count > 0, "log file should have at least some messages");

    remove(logfile);
    PASS();
}

/* ── Test 5: NULL safety ─────────────────────────────────────── */

static void test_concurrent_null_safety(void) {
    TEST("Concurrency: all APIs handle NULL gracefully");

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
    repositoryGetSessionsByMode(NULL, "strict", &count);
    repositoryDestroy(NULL);

    loggerLog(NULL, LOG_LEVEL_INFO, "test");
    loggerSetLevel(NULL, LOG_LEVEL_DEBUG);
    loggerGetLevel(NULL);
    loggerLogToStdout(NULL, false);
    loggerAddFile(NULL, "test.log");
    loggerDestroy(NULL);

    PASS();
}

/* ── Main ────────────────────────────────────────────────────── */

int main(void) {
    printf("=== ctypr Concurrency Test Suite ===\n\n");

    setupTestDbPath();
    test_concurrent_saves();
    test_concurrent_read_write();
    test_concurrent_logger();
    test_concurrent_logger_reconfigure();
    test_concurrent_null_safety();

    TEST_SUMMARY();
}
