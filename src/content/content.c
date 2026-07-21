#include "content.h"
#include "formatter.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sqlite3.h>

#define CONTENT_DB_DEFAULT_LIMIT 300

typedef enum ContentProviderType {
    CONTENT_PROVIDER_STRING,
    CONTENT_PROVIDER_FILE,
    CONTENT_PROVIDER_DATABASE,
    CONTENT_PROVIDER_WEB
} ContentProviderType;

typedef struct ContentProvider {
    char text[4096];
    size_t length;
    size_t currentIndex;

    char* filepath;
    char* url;
    ContentProviderType type;
    Logger* logger;
    const char* name;

    ContentMode mode;
    size_t contentLimit;
    bool contentFetched;
} ContentProvider;

ContentProvider* contentProviderFromString(const char* text) {
    if (!text) return NULL;
    ContentProvider* cp = malloc(sizeof(ContentProvider));
    if (!cp) {
        fprintf(stderr, "[ERROR] Failed to allocate ContentProvider (string)\n");
        return NULL;
    }
    snprintf(cp->text, sizeof(cp->text), "%s", text);
    cp->length = strlen(cp->text);
    cp->currentIndex = 0;
    cp->type = CONTENT_PROVIDER_STRING;
    cp->filepath = NULL;
    cp->url = NULL;
    cp->logger = NULL;
    cp->name = "StringProvider";
    cp->mode = CONTENT_MODE_SENTENCES;
    cp->contentLimit = CONTENT_DB_DEFAULT_LIMIT;
    cp->contentFetched = false;
    return cp;
}

ContentProvider* contentProviderFromFile(const char* filepath) {
    if (!filepath) return NULL;
    ContentProvider* cp = malloc(sizeof(ContentProvider));
    if (!cp) {
        fprintf(stderr, "[ERROR] Failed to allocate ContentProvider (file)\n");
        return NULL;
    }

    cp->filepath = strdup(filepath);
    if (!cp->filepath) {
        fprintf(stderr, "[ERROR] Failed to allocate filepath for ContentProvider\n");
        free(cp);
        return NULL;
    }
    cp->text[0] = '\0';
    cp->length = 0;
    cp->currentIndex = 0;
    cp->type = CONTENT_PROVIDER_FILE;
    cp->url = NULL;
    cp->logger = NULL;
    cp->name = "FileProvider";
    cp->mode = CONTENT_MODE_SENTENCES;
    cp->contentLimit = CONTENT_DB_DEFAULT_LIMIT;
    cp->contentFetched = false;
    return cp;
}

ContentProvider* contentProviderFromDatabase(const char* filepath) {
    if (!filepath) return NULL;
    ContentProvider* cp = malloc(sizeof(ContentProvider));
    if (!cp) {
        fprintf(stderr, "[ERROR] Failed to allocate ContentProvider (database)\n");
        return NULL;
    }

    cp->filepath = strdup(filepath);
    if (!cp->filepath) {
        fprintf(stderr, "[ERROR] Failed to allocate filepath for ContentProvider\n");
        free(cp);
        return NULL;
    }
    cp->text[0] = '\0';
    cp->length = 0;
    cp->currentIndex = 0;
    cp->type = CONTENT_PROVIDER_DATABASE;
    cp->url = NULL;
    cp->logger = NULL;
    cp->name = "DbProvider";
    cp->mode = CONTENT_MODE_COMMON_WORDS;
    cp->contentLimit = CONTENT_DB_DEFAULT_LIMIT;
    cp->contentFetched = false;
    return cp;
}

ContentProvider* contentProviderFromWeb(const char* url) {
    if (!url) return NULL;
    ContentProvider* cp = malloc(sizeof(ContentProvider));
    if (!cp) {
        fprintf(stderr, "[ERROR] Failed to allocate ContentProvider (web)\n");
        return NULL;
    }

    cp->url = strdup(url);
    if (!cp->url) {
        fprintf(stderr, "[ERROR] Failed to allocate URL for ContentProvider\n");
        free(cp);
        return NULL;
    }
    snprintf(cp->text, sizeof(cp->text), "%s", "The quick brown fox jumps over the lazy dog.");
    cp->length = strlen(cp->text);
    cp->currentIndex = 0;
    cp->type = CONTENT_PROVIDER_WEB;
    cp->filepath = NULL;
    cp->logger = NULL;
    cp->name = "WebProvider";
    cp->mode = CONTENT_MODE_SENTENCES;
    cp->contentLimit = CONTENT_DB_DEFAULT_LIMIT;
    cp->contentFetched = false;
    return cp;
}

void contentProviderSetLogger(ContentProvider* self, Logger* logger) {
    if (self) self->logger = logger;
}

void contentProviderSetMode(ContentProvider* self, ContentMode mode) {
    if (self) self->mode = mode;
}

void contentProviderSetContentLimit(ContentProvider* self, size_t limit) {
    if (self) self->contentLimit = limit;
}

void contentProviderDestroy(ContentProvider* provider) {
    if (provider) {
        if (provider->logger) loggerLog(provider->logger, LOG_LEVEL_DEBUG, "ContentProvider destroyed");
        free(provider->filepath);
        free(provider->url);
        free(provider);
    }
}

static ContentChunk _cpGetString(ContentProvider* cp) {
    ContentChunk c;
    snprintf(c.text, sizeof(c.text), "%s", cp->text);
    c.length = strlen(c.text);
    return c;
}

static ContentChunk _cpGetFile(ContentProvider* cp) {
    ContentChunk c;
    memset(&c, 0, sizeof(c));

    if (!cp->filepath) {
        if (cp->logger) loggerLog(cp->logger, LOG_LEVEL_WARNING, "File provider: no filepath set");
        return c;
    }

    FILE* f = fopen(cp->filepath, "rb");
    if (!f) {
        if (cp->logger) loggerLog(cp->logger, LOG_LEVEL_WARNING, "File provider: could not open file");
        return c;
    }

    size_t total = 0;
    size_t n = 0;
    bool truncated = false;
    while (total < sizeof(cp->text) - 1 && (n = fread(cp->text + total, 1, sizeof(cp->text) - 1 - total, f)) > 0) {
        total += n;
        if (total >= sizeof(cp->text) - 1) {
            truncated = true;
        }
    }
    if (truncated && cp->logger) {
        loggerLog(cp->logger, LOG_LEVEL_WARNING, "File provider: file exceeds buffer, truncated to 4095 bytes");
    }
    cp->text[total] = '\0';
    cp->length = total;
    cp->currentIndex = 0;

    fclose(f);

    if (cp->mode == CONTENT_MODE_RANDOM_WORDS) {
        char* words[2048];
        int wordCount = 0;
        char* token = strtok(cp->text, " \t\n\r");
        while (token && wordCount < 2048) {
            words[wordCount++] = token;
            token = strtok(NULL, " \t\n\r");
        }
        for (int i = wordCount - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            char* tmp = words[i];
            words[i] = words[j];
            words[j] = tmp;
        }
        size_t pos = 0;
        for (int i = 0; i < wordCount; i++) {
            size_t wlen = strlen(words[i]);
            if (pos + wlen + 1 >= sizeof(cp->text)) break;
            if (i > 0) cp->text[pos++] = ' ';
            memcpy(cp->text + pos, words[i], wlen);
            pos += wlen;
        }
        cp->text[pos] = '\0';
        cp->length = pos;
    } else if (cp->mode == CONTENT_MODE_SENTENCES && cp->length > 256) {
        Formatter* fmt = formatterCreate();
        if (fmt) {
            if (cp->logger) formatterSetLogger(fmt, cp->logger);
            ContentChunk fmtChunk = formatterFormat(fmt, cp->text, 256);
            snprintf(cp->text, sizeof(cp->text), "%s", fmtChunk.text);
            cp->length = fmtChunk.length;
            formatterDestroy(fmt);
        }
    }

    if (cp->logger) loggerLog(cp->logger, LOG_LEVEL_DEBUG, "File provider: read file");

    snprintf(c.text, sizeof(c.text), "%s", cp->text);
    c.length = cp->length;
    return c;
}

static bool dbAppendResult(sqlite3_stmt* stmt, char* buffer, size_t bufferSize, size_t* offset) {
    const unsigned char* text = sqlite3_column_text(stmt, 0);
    if (!text) return true;

    size_t textLen = strlen((const char*)text);
    if (*offset + textLen + 1 >= bufferSize) return false;

    if (*offset > 0) {
        buffer[(*offset)++] = ' ';
    }
    memcpy(buffer + *offset, text, textLen);
    *offset += textLen;
    buffer[*offset] = '\0';
    return true;
}

static ContentChunk _cpGetDatabase(ContentProvider* cp) {
    ContentChunk c;
    memset(&c, 0, sizeof(c));

    if (!cp->filepath) {
        if (cp->logger) loggerLog(cp->logger, LOG_LEVEL_WARNING, "Database provider: no filepath set");
        return c;
    }

    if (cp->contentFetched) {
        if (cp->logger) loggerLog(cp->logger, LOG_LEVEL_DEBUG, "Database provider: content already fetched");
        return c;
    }

    sqlite3* db = NULL;
    int rc = sqlite3_open_v2(cp->filepath, &db, SQLITE_OPEN_READONLY, NULL);
    if (rc != SQLITE_OK || !db) {
        if (cp->logger) loggerLog(cp->logger, LOG_LEVEL_WARNING, "Database provider: could not open database");
        if (db) sqlite3_close(db);
        return c;
    }

    const char* query = NULL;
    switch (cp->mode) {
        case CONTENT_MODE_COMMON_WORDS:
            query = "SELECT word FROM common_words ORDER BY frequency_rank ASC LIMIT ?";
            break;
        case CONTENT_MODE_RANDOM_WORDS:
            query = "SELECT word FROM random_words ORDER BY RANDOM() LIMIT ?";
            break;
        case CONTENT_MODE_SENTENCES:
            query = "SELECT text_content FROM typing_sentences LIMIT ?";
            break;
        default:
            if (cp->logger) loggerLog(cp->logger, LOG_LEVEL_WARNING, "Database provider: unknown content mode, defaulting to sentences");
            query = "SELECT text_content FROM typing_sentences LIMIT ?";
            break;
    }

    sqlite3_stmt* stmt = NULL;
    rc = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        if (cp->logger) loggerLog(cp->logger, LOG_LEVEL_WARNING, "Database provider: query prepare failed");
        sqlite3_close(db);
        return c;
    }

    rc = sqlite3_bind_int64(stmt, 1, (int64_t)cp->contentLimit);
    if (rc != SQLITE_OK) {
        if (cp->logger) loggerLog(cp->logger, LOG_LEVEL_WARNING, "Database provider: bind failed");
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return c;
    }

    size_t offset = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!dbAppendResult(stmt, cp->text, sizeof(cp->text), &offset)) {
            break;
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    cp->length = offset;
    cp->currentIndex = 0;
    cp->contentFetched = true;

    if (cp->logger) {
        if (offset > 0) {
            loggerLog(cp->logger, LOG_LEVEL_DEBUG, "Database provider: content fetched");
        } else {
            loggerLog(cp->logger, LOG_LEVEL_WARNING, "Database provider: query returned no rows");
        }
    }

    snprintf(c.text, sizeof(c.text), "%s", cp->text);
    c.length = cp->length;
    return c;
}

static ContentChunk _cpGetWeb(ContentProvider* cp) {
    ContentChunk c;
    memset(&c, 0, sizeof(c));

    if (cp->logger) loggerLog(cp->logger, LOG_LEVEL_WARNING, "Web ContentProvider not implemented, using fallback");

    snprintf(cp->text, sizeof(cp->text), "%s", "The quick brown fox jumps over the lazy dog.");
    cp->length = strlen(cp->text);
    cp->currentIndex = 0;

    snprintf(c.text, sizeof(c.text), "%s", cp->text);
    c.length = cp->length;
    return c;
}

ContentChunk contentProviderGetNext(ContentProvider *provider) {
    ContentChunk c;
    memset(&c, 0, sizeof(c));
    if (!provider) return c;
    if (provider->type == CONTENT_PROVIDER_STRING)   return _cpGetString(provider);
    if (provider->type == CONTENT_PROVIDER_FILE)     return _cpGetFile(provider);
    if (provider->type == CONTENT_PROVIDER_DATABASE) return _cpGetDatabase(provider);
    if (provider->type == CONTENT_PROVIDER_WEB)      return _cpGetWeb(provider);
    return c;
}

void contentProviderReset(ContentProvider* provider) {
    if (!provider) return;
    provider->currentIndex = 0;
    provider->contentFetched = false;
}

bool contentProviderIsExhausted(ContentProvider* provider) {
    if (!provider) return true;
    return provider->currentIndex >= provider->length;
}
