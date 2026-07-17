#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
typedef SRWLOCK ctypr_mutex_t;
#define MUTEX_INIT(m)   InitializeSRWLock(m)
#define MUTEX_LOCK(m)   AcquireSRWLockExclusive(m)
#define MUTEX_UNLOCK(m) ReleaseSRWLockExclusive(m)
#define MUTEX_DESTROY(m) ((void)0)
#else
#include <pthread.h>
typedef pthread_mutex_t ctypr_mutex_t;
#define MUTEX_INIT(m)   pthread_mutex_init(m, NULL)
#define MUTEX_LOCK(m)   pthread_mutex_lock(m)
#define MUTEX_UNLOCK(m) pthread_mutex_unlock(m)
#define MUTEX_DESTROY(m) pthread_mutex_destroy(m)
#endif

struct Logger {
    LogLevel currentLevel;
    bool stdoutEnabled;
    FILE* files[LOGGER_MAX_FILES];
    int fileCount;
    ctypr_mutex_t lock;
};

static const char* LEVEL_LABELS[] = {
    "DEBUG", "INFO", "WARNING", "ERROR", "NONE"
};

Logger* loggerCreate(LogLevel level, bool enableStdout) {
    Logger* logger = malloc(sizeof(Logger));
    if (!logger) return NULL;
    logger->currentLevel = level;
    logger->stdoutEnabled = enableStdout;
    memset(logger->files, 0, sizeof(logger->files));
    logger->fileCount = 0;
    MUTEX_INIT(&logger->lock);
    return logger;
}

void loggerDestroy(Logger* logger) {
    if (!logger) return;
    MUTEX_LOCK(&logger->lock);
    for (size_t i = 0; i < (size_t)logger->fileCount; i++) {
        fclose(logger->files[i]);
    }
    MUTEX_UNLOCK(&logger->lock);
    MUTEX_DESTROY(&logger->lock);
    free(logger);
}

void loggerSetLevel(Logger *logger, LogLevel level) {
    if (!logger) return;
    logger->currentLevel = level;
}

LogLevel loggerGetLevel(Logger *logger) {
    if (!logger) return LOG_LEVEL_NONE;
    return logger->currentLevel;
}

void loggerLogToStdout(Logger* logger, bool enable) {
    if (!logger) return;
    logger->stdoutEnabled = enable;
}

bool loggerAddFile(Logger *logger, const char *filepath) {
    if (!logger) return false;
    bool ret = true;
    MUTEX_LOCK(&logger->lock);
    if (logger->fileCount >= LOGGER_MAX_FILES) {
        ret = false;
    } else {
        FILE* file = fopen(filepath, "a");
        if (!file) ret = false;
        else logger->files[logger->fileCount++] = file;
    }
    MUTEX_UNLOCK(&logger->lock);
    return ret;
}

void loggerLog(Logger* logger, LogLevel level, const char* message) {
    if (!logger) return;
    if (level < logger->currentLevel || level > LOG_LEVEL_ERROR) return;
    MUTEX_LOCK(&logger->lock);
    if (logger->stdoutEnabled) {
        fprintf(stdout, "[%s] %s\n", LEVEL_LABELS[level], message);
        fflush(stdout);
    }
    for (size_t i = 0; i < (size_t)logger->fileCount; i++) {
        fprintf(logger->files[i], "[%s] %s\n", LEVEL_LABELS[level], message);
        fflush(logger->files[i]);
    }
    MUTEX_UNLOCK(&logger->lock);
}
