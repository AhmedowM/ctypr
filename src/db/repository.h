#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <stdint.h>
#include <stdbool.h>

#include <stddef.h>

typedef struct Repository Repository;
typedef struct Logger Logger;

/// @brief Persisted session data stored in the database.
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

// ── Lifecycle ────────────────────────────────────────────────────────────────

/// @brief Open or create a Repository backed by a SQLite database file.
/// @param dbPath Path to the SQLite database file.
/// @return A new Repository instance, or NULL on allocation failure.
Repository* repositoryCreate(const char* dbPath);

/// @brief Close the database and destroy the Repository.
/// @param repo The Repository to destroy (NULL is safe).
void repositoryDestroy(Repository* repo);

// ── Logger ───────────────────────────────────────────────────────────────────

/// @brief Attach a Logger to the repository for diagnostic output.
/// @param self   The Repository instance.
/// @param logger The Logger instance to attach (NULL to detach).
void repositorySetLogger(Repository* self, Logger* logger);

// ── CRUD Operations ──────────────────────────────────────────────────────────

/// @brief Save a session record to the database.
/// @param repo The Repository instance.
/// @param data The session data to persist.
/// @return The auto-generated session ID on success, or -1 on failure.
int64_t repositorySaveSession(Repository* repo, const SessionData* data);

/// @brief Retrieve a session by its ID.
/// @param repo The Repository instance.
/// @param id   The session ID to look up.
/// @return The session data if found, or a zeroed struct (id == 0) if not found.
SessionData repositoryGetSession(Repository* repo, int64_t id);

/// @brief Retrieve all sessions ordered by most recent first.
/// @param repo  The Repository instance.
/// @param count Output parameter: the number of sessions returned.
/// @return A heap-allocated array of SessionData (caller must free), or NULL on failure.
SessionData* repositoryGetAll(Repository* repo, size_t* count);

/// @brief Retrieve the most recent N sessions.
/// @param repo  The Repository instance.
/// @param limit Maximum number of sessions to return.
/// @param count Output parameter: the number of sessions returned.
/// @return A heap-allocated array of SessionData (caller must free), or NULL on failure.
SessionData* repositoryGetRecent(Repository* repo, int64_t limit, size_t* count);

/// @brief Get the total number of sessions in the database.
/// @param repo The Repository instance.
/// @return The session count, or 0 on failure or if repo is NULL.
int64_t repositoryGetCount(Repository* repo);

/// @brief Delete a single session by ID.
/// @param repo The Repository instance.
/// @param id   The session ID to delete.
/// @return true if a session was deleted, false if not found or on failure.
bool repositoryDeleteSession(Repository* repo, int64_t id);

/// @brief Delete all sessions from the database.
/// @param repo The Repository instance (NULL is safe).
void repositoryClearAll(Repository* repo);

// ── Query Operations ─────────────────────────────────────────────────────────

/// @brief Get the session with the highest WPM.
/// @param repo The Repository instance.
/// @return The session data with the highest WPM, or a zeroed struct if no sessions exist.
SessionData repositoryGetBestWpm(Repository* repo);

/// @brief Get the session with the highest raw WPM.
/// @param repo The Repository instance.
/// @return The session data with the highest raw WPM, or a zeroed struct if no sessions exist.
SessionData repositoryGetBestRawWpm(Repository* repo);

/// @brief Calculate the average WPM across all sessions.
/// @param repo The Repository instance.
/// @return The average WPM, or 0.0 if no sessions exist.
double repositoryGetAverageWpm(Repository* repo);

#endif // REPOSITORY_H
