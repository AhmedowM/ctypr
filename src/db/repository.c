#include "repository.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sqlite3.h>

typedef struct Repository {
    char* dbPath;
    int initialized;
} Repository;

static const char* SCHEMA = R"(
    CREATE TABLE IF NOT EXISTS sessions (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        timestamp TEXT NOT NULL,
        mode TEXT NOT NULL,
        total_chars INTEGER NOT NULL,
        correct_chars INTEGER NOT NULL,
        duration_ms INTEGER NOT NULL,
        wpm REAL NOT NULL,
        wpm_raw REAL NOT NULL,
        accuracy REAL NOT NULL
    );
    CREATE INDEX IF NOT EXISTS idx_sessions_mode ON sessions(mode);
    CREATE INDEX IF NOT EXISTS idx_sessions_timestamp ON sessions(timestamp);
)";

static char* getFullPath(const char* dbPath) {
    if (!dbPath || strcmp(dbPath, "~/.typr/typr.db") == 0) {
        // Fallback to current directory
        return strdup("typr.db");
    }
    return strdup(dbPath);
}

static int ensureInitialized(Repository* repo) {
    if (repo->initialized) return 0;
    
    char* fullPath = getFullPath(repo->dbPath);
    if (!fullPath) return -1;
    
    sqlite3* db = NULL;
    int rc = sqlite3_open(fullPath, &db);
    free(fullPath);
    
    if (rc != SQLITE_OK || db == NULL) {
        if (db) sqlite3_close(db);
        return -1;
    }
    
    char* errMsg = NULL;
    rc = sqlite3_exec(db, SCHEMA, NULL, NULL, &errMsg);
    if (rc != SQLITE_OK) {
        if (errMsg) sqlite3_free(errMsg);
        sqlite3_close(db);
        return -1;
    }
    
    sqlite3_close(db);
    repo->initialized = 1;
    return 0;
}

Repository* repositoryCreate(const char* dbPath) {
    Repository* repo = malloc(sizeof(Repository));
    if (!repo) return NULL;
    
    repo->dbPath = strdup(dbPath ? dbPath : "typr.db");
    repo->initialized = 0;
    
    return repo;
}

void repositoryDestroy(Repository* repo) {
    if (repo) {
        free(repo->dbPath);
        free(repo);
    }
}

static int rowToSessionData(sqlite3_stmt* stmt, SessionData* data) {
    if (!data) return -1;
    
    data->id = sqlite3_column_int64(stmt, 0);
    const char* timestamp = (const char*)sqlite3_column_text(stmt, 1);
    const char* mode = (const char*)sqlite3_column_text(stmt, 2);
    data->totalChars = sqlite3_column_int64(stmt, 3);
    data->correctChars = sqlite3_column_int64(stmt, 4);
    data->durationMs = sqlite3_column_int64(stmt, 5);
    data->wpm = sqlite3_column_double(stmt, 6);
    data->wpmRaw = sqlite3_column_double(stmt, 7);
    data->accuracy = sqlite3_column_double(stmt, 8);
    
    if (timestamp) {
        snprintf(data->timestamp, sizeof(data->timestamp), "%s", timestamp);
    } else {
        data->timestamp[0] = '\0';
    }
    
    if (mode) {
        snprintf(data->mode, sizeof(data->mode), "%s", mode);
    } else {
        data->mode[0] = '\0';
    }
    
    return 0;
}

int64_t repositorySaveSession(Repository* repo, const SessionData* data) {
    if (!repo || !data) return -1;
    if (ensureInitialized(repo) != 0) return -1;
    
    char* fullPath = getFullPath(repo->dbPath);
    if (!fullPath) return -1;
    
    sqlite3* db = NULL;
    int rc = sqlite3_open(fullPath, &db);
    free(fullPath);
    
    if (rc != SQLITE_OK || db == NULL) {
        if (db) sqlite3_close(db);
        return -1;
    }
    
    const char* sql = "INSERT INTO sessions (timestamp, mode, total_chars, correct_chars, duration_ms, wpm, wpm_raw, accuracy) VALUES (?, ?, ?, ?, ?, ?, ?, ?)";
    
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, data->timestamp, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, data->mode, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, data->totalChars);
    sqlite3_bind_int64(stmt, 4, data->correctChars);
    sqlite3_bind_int64(stmt, 5, data->durationMs);
    sqlite3_bind_double(stmt, 6, data->wpm);
    sqlite3_bind_double(stmt, 7, data->wpmRaw);
    sqlite3_bind_double(stmt, 8, data->accuracy);
    
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) {
        int64_t id = sqlite3_last_insert_rowid(db);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return id;
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return -1;
}

SessionData repositoryGetSession(Repository* repo, int64_t id) {
    SessionData data;
    memset(&data, 0, sizeof(data));
    
    if (!repo) return data;
    if (ensureInitialized(repo) != 0) return data;
    
    char* fullPath = getFullPath(repo->dbPath);
    if (!fullPath) return data;
    
    sqlite3* db = NULL;
    int rc = sqlite3_open(fullPath, &db);
    free(fullPath);
    
    if (rc != SQLITE_OK || db == NULL) {
        if (db) sqlite3_close(db);
        return data;
    }
    
    const char* sql = "SELECT id, timestamp, mode, total_chars, correct_chars, duration_ms, wpm, wpm_raw, accuracy FROM sessions WHERE id = ?";
    
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return data;
    }
    
    sqlite3_bind_int64(stmt, 1, id);
    
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        rowToSessionData(stmt, &data);
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    
    return data;
}

SessionData* repositoryGetAll(Repository* repo, size_t* count) {
    if (!repo || !count) return NULL;
    if (ensureInitialized(repo) != 0) {
        *count = 0;
        return NULL;
    }
    
    char* fullPath = getFullPath(repo->dbPath);
    if (!fullPath) {
        *count = 0;
        return NULL;
    }
    
    sqlite3* db = NULL;
    int rc = sqlite3_open(fullPath, &db);
    free(fullPath);
    
    if (rc != SQLITE_OK || db == NULL) {
        if (db) sqlite3_close(db);
        *count = 0;
        return NULL;
    }
    
    const char* sql = "SELECT id, timestamp, mode, total_chars, correct_chars, duration_ms, wpm, wpm_raw, accuracy FROM sessions ORDER BY id DESC";
    
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        *count = 0;
        return NULL;
    }
    
    // Count rows first
    size_t capacity = 16;
    size_t n = 0;
    SessionData* results = malloc(capacity * sizeof(SessionData));
    if (!results) {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        *count = 0;
        return NULL;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (n >= capacity) {
            capacity *= 2;
            SessionData* tmp = realloc(results, capacity * sizeof(SessionData));
            if (!tmp) {
                free(results);
                sqlite3_finalize(stmt);
                sqlite3_close(db);
                *count = 0;
                return NULL;
            }
            results = tmp;
        }
        rowToSessionData(stmt, &results[n]);
        n++;
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    
    *count = n;
    return results;
}

SessionData* repositoryGetRecent(Repository* repo, int64_t limit, size_t* count) {
    if (!repo || !count) return NULL;
    if (ensureInitialized(repo) != 0) {
        *count = 0;
        return NULL;
    }
    
    char* fullPath = getFullPath(repo->dbPath);
    if (!fullPath) {
        *count = 0;
        return NULL;
    }
    
    sqlite3* db = NULL;
    int rc = sqlite3_open(fullPath, &db);
    free(fullPath);
    
    if (rc != SQLITE_OK || db == NULL) {
        if (db) sqlite3_close(db);
        *count = 0;
        return NULL;
    }
    
    const char* sql = "SELECT id, timestamp, mode, total_chars, correct_chars, duration_ms, wpm, wpm_raw, accuracy FROM sessions ORDER BY id DESC LIMIT ?";
    
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        *count = 0;
        return NULL;
    }
    
    sqlite3_bind_int64(stmt, 1, limit);
    
    size_t n = 0;
    size_t capacity = 16;
    SessionData* results = malloc(capacity * sizeof(SessionData));
    if (!results) {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        *count = 0;
        return NULL;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (n >= capacity) {
            capacity *= 2;
            SessionData* tmp = realloc(results, capacity * sizeof(SessionData));
            if (!tmp) {
                free(results);
                sqlite3_finalize(stmt);
                sqlite3_close(db);
                *count = 0;
                return NULL;
            }
            results = tmp;
        }
        rowToSessionData(stmt, &results[n]);
        n++;
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    
    *count = n;
    return results;
}

int64_t repositoryGetCount(Repository* repo) {
    if (!repo) return 0;
    if (ensureInitialized(repo) != 0) return 0;
    
    char* fullPath = getFullPath(repo->dbPath);
    if (!fullPath) return 0;
    
    sqlite3* db = NULL;
    int rc = sqlite3_open(fullPath, &db);
    free(fullPath);
    
    if (rc != SQLITE_OK || db == NULL) {
        if (db) sqlite3_close(db);
        return 0;
    }
    
    const char* sql = "SELECT COUNT(*) FROM sessions";
    
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return 0;
    }
    
    int64_t count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int64(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    
    return count;
}

bool repositoryDeleteSession(Repository* repo, int64_t id) {
    if (!repo) return false;
    if (ensureInitialized(repo) != 0) return false;
    
    char* fullPath = getFullPath(repo->dbPath);
    if (!fullPath) return false;
    
    sqlite3* db = NULL;
    int rc = sqlite3_open(fullPath, &db);
    free(fullPath);
    
    if (rc != SQLITE_OK || db == NULL) {
        if (db) sqlite3_close(db);
        return false;
    }
    
    const char* sql = "DELETE FROM sessions WHERE id = ?";
    
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, id);
    
    rc = sqlite3_step(stmt);
    bool success = (rc == SQLITE_DONE && sqlite3_changes(db) > 0);
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    
    return success;
}

void repositoryClearAll(Repository* repo) {
    if (!repo) return;
    if (ensureInitialized(repo) != 0) return;
    
    char* fullPath = getFullPath(repo->dbPath);
    if (!fullPath) return;
    
    sqlite3* db = NULL;
    int rc = sqlite3_open(fullPath, &db);
    free(fullPath);
    
    if (rc != SQLITE_OK || db == NULL) {
        if (db) sqlite3_close(db);
        return;
    }
    
    const char* sql = "DELETE FROM sessions";
    
    char* errMsg = NULL;
    rc = sqlite3_exec(db, sql, NULL, NULL, &errMsg);
    if (rc != SQLITE_OK) {
        if (errMsg) sqlite3_free(errMsg);
    }
    
    sqlite3_close(db);
}

SessionData repositoryGetBestWpm(Repository* repo) {
    SessionData data;
    memset(&data, 0, sizeof(data));
    
    if (!repo) return data;
    if (ensureInitialized(repo) != 0) return data;
    
    char* fullPath = getFullPath(repo->dbPath);
    if (!fullPath) return data;
    
    sqlite3* db = NULL;
    int rc = sqlite3_open(fullPath, &db);
    free(fullPath);
    
    if (rc != SQLITE_OK || db == NULL) {
        if (db) sqlite3_close(db);
        return data;
    }
    
    const char* sql = "SELECT id, timestamp, mode, total_chars, correct_chars, duration_ms, wpm, wpm_raw, accuracy FROM sessions ORDER BY wpm DESC LIMIT 1";
    
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return data;
    }
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        rowToSessionData(stmt, &data);
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    
    return data;
}

SessionData repositoryGetBestRawWpm(Repository* repo) {
    SessionData data;
    memset(&data, 0, sizeof(data));
    
    if (!repo) return data;
    if (ensureInitialized(repo) != 0) return data;
    
    char* fullPath = getFullPath(repo->dbPath);
    if (!fullPath) return data;
    
    sqlite3* db = NULL;
    int rc = sqlite3_open(fullPath, &db);
    free(fullPath);
    
    if (rc != SQLITE_OK || db == NULL) {
        if (db) sqlite3_close(db);
        return data;
    }
    
    const char* sql = "SELECT id, timestamp, mode, total_chars, correct_chars, duration_ms, wpm, wpm_raw, accuracy FROM sessions ORDER BY wpm_raw DESC LIMIT 1";
    
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return data;
    }
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        rowToSessionData(stmt, &data);
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    
    return data;
}

double repositoryGetAverageWpm(Repository* repo) {
    if (!repo) return 0.0;
    if (ensureInitialized(repo) != 0) return 0.0;
    
    char* fullPath = getFullPath(repo->dbPath);
    if (!fullPath) return 0.0;
    
    sqlite3* db = NULL;
    int rc = sqlite3_open(fullPath, &db);
    free(fullPath);
    
    if (rc != SQLITE_OK || db == NULL) {
        if (db) sqlite3_close(db);
        return 0.0;
    }
    
    const char* sql = "SELECT AVG(wpm) FROM sessions";
    
    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return 0.0;
    }
    
    double avg = 0.0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        avg = sqlite3_column_double(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    
    return avg;
}