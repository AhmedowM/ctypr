#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <stdint.h>
#include <stdbool.h>

typedef struct SessionData {
    int64_t id;
    char timestamp[20];
    char mode[16];
    int64_t totalChars;
    int64_t correctChars;
    int64_t durationMs;
    double wpm;
    double wpmRaw;
    double accuracy;
} SessionData;

typedef struct Repository Repository;

Repository* repositoryCreate(const char* dbPath);
void repositoryDestroy(Repository* repo);

bool repositorySaveSession(Repository* repo, const SessionData* data);
SessionData repositoryGetSession(Repository* repo, int64_t id);
SessionData* repositoryGetAll(Repository* repo, size_t* count);
SessionData* repositoryGetRecent(Repository* repo, int64_t limit, size_t* count);
int64_t repositoryGetCount(Repository* repo);
bool repositoryDeleteSession(Repository* repo, int64_t id);
void repositoryClearAll(Repository* repo);

SessionData repositoryGetBestWpm(Repository* repo);
SessionData repositoryGetBestRawWpm(Repository* repo);
double repositoryGetAverageWpm(Repository* repo);

#endif // REPOSITORY_H