#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Repository Repository;
typedef struct Logger Logger;

/// @brief Persisted session data.
typedef struct SessionData {
    int64_t id;             ///< Unique session identifier (auto-generated)
    char timestamp[20];     ///< ISO 8601 timestamp (YYYY-MM-DD HH:MM:SS)
    char mode[16];          ///< Engine mode ("strict" or "flow")
    int64_t totalChars;     ///< Total characters typed
    int64_t correctChars;   ///< Correctly typed characters
    int64_t durationMs;     ///< Session duration in milliseconds
    double wpm;             ///< Adjusted words per minute
    double wpmRaw;          ///< Raw words per minute
    double accuracy;        ///< Accuracy percentage
} SessionData;

/// @brief Thread safety: Repository is thread-safe. Internal locking protects
///        all public operations. Callers must ensure no other thread is using
///        the same instance during repositoryDestroy().

// ── Lifecycle ────────────────────────────────────────────────────────────────

/// @brief Open or create a Repository backed by SQLite.
/// @param dbPath Path to SQLite database file.
/// @return New Repository, or NULL on allocation failure.
Repository* repositoryCreate(const char* dbPath);

/// @brief Close database and destroy Repository.
/// @param repo Repository to destroy (NULL is safe).
void repositoryDestroy(Repository* repo);

// ── Logger ───────────────────────────────────────────────────────────────────

/// @brief Attach a Logger for diagnostic output.
/// @param self   Repository instance.
/// @param logger Logger to attach (NULL to detach).
void repositorySetLogger(Repository* self, Logger* logger);

// ── CRUD Operations ──────────────────────────────────────────────────────────

/// @brief Save a session record.
/// @param repo Repository instance.
/// @param data Session data to persist.
/// @return Auto-generated session ID on success, or -1 on failure.
int64_t repositorySaveSession(Repository* repo, const SessionData* data);

/// @brief Retrieve a session by ID.
/// @param repo Repository instance.
/// @param id   Session ID to look up.
/// @return Session data if found, or zeroed struct (id == 0) if not found.
SessionData repositoryGetSession(Repository* repo, int64_t id);

/// @brief Retrieve all sessions, most recent first.
/// @param repo  Repository instance.
/// @param count Output: number of sessions returned.
/// @return Heap-allocated array of SessionData (caller frees), or NULL on failure.
SessionData* repositoryGetAll(Repository* repo, size_t* count);

/// @brief Retrieve the most recent N sessions.
/// @param repo  Repository instance.
/// @param limit Maximum sessions to return.
/// @param count Output: number of sessions returned.
/// @return Heap-allocated array of SessionData (caller frees), or NULL on failure.
SessionData* repositoryGetRecent(Repository* repo, int64_t limit, size_t* count);

/// @brief Get total session count.
/// @param repo Repository instance.
/// @return Session count, or 0 on failure or if repo is NULL.
int64_t repositoryGetCount(Repository* repo);

/// @brief Delete a session by ID.
/// @param repo Repository instance.
/// @param id   Session ID to delete.
/// @return true if deleted, false if not found or on failure.
bool repositoryDeleteSession(Repository* repo, int64_t id);

/// @brief Delete all sessions.
/// @param repo Repository instance (NULL is safe).
void repositoryClearAll(Repository* repo);

// ── Query Operations ─────────────────────────────────────────────────────────

/// @brief Get session with highest WPM.
/// @param repo Repository instance.
/// @return Session with highest WPM, or zeroed struct if no sessions.
SessionData repositoryGetBestWpm(Repository* repo);

/// @brief Get session with highest raw WPM.
/// @param repo Repository instance.
/// @return Session with highest raw WPM, or zeroed struct if no sessions.
SessionData repositoryGetBestRawWpm(Repository* repo);

/// @brief Calculate average WPM across all sessions.
/// @param repo Repository instance.
/// @return Average WPM, or 0.0 if no sessions.
double repositoryGetAverageWpm(Repository* repo);

/// @brief Retrieve sessions filtered by mode, most recent first.
/// @param repo  Repository instance.
/// @param mode  Mode string ("strict" or "flow").
/// @param count Output: number of sessions returned.
/// @return Heap-allocated array of SessionData (caller frees), or NULL on failure.
SessionData* repositoryGetSessionsByMode(Repository* repo, const char* mode, size_t* count);

#ifdef __cplusplus
}
#endif

#endif // REPOSITORY_H