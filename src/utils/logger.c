#include "logger.h"
#include "platform_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    if (!logger) {
        fprintf(stderr, "[ERROR] Failed to allocate Logger\n");
        return NULL;
    }
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
    MUTEX_LOCK(&logger->lock);
    logger->currentLevel = level;
    MUTEX_UNLOCK(&logger->lock);
}

LogLevel loggerGetLevel(Logger *logger) {
    if (!logger) return LOG_LEVEL_NONE;
    MUTEX_LOCK(&logger->lock);
    LogLevel level = logger->currentLevel;
    MUTEX_UNLOCK(&logger->lock);
    return level;
}

void loggerLogToStdout(Logger* logger, bool enable) {
    if (!logger) return;
    MUTEX_LOCK(&logger->lock);
    logger->stdoutEnabled = enable;
    MUTEX_UNLOCK(&logger->lock);
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
    MUTEX_LOCK(&logger->lock);
    if (level < logger->currentLevel || level == LOG_LEVEL_NONE) {
        MUTEX_UNLOCK(&logger->lock);
        return;
    }
    if (logger->stdoutEnabled) {
        fprintf(stdout, "[%s] %s\n", LEVEL_LABELS[level], message);
        fflush(stdout);
    }
    for (int i = 0; i < logger->fileCount; i++) {
        fprintf(logger->files[i], "[%s] %s\n", LEVEL_LABELS[level], message);
        fflush(logger->files[i]);
    }
    MUTEX_UNLOCK(&logger->lock);
}
