#ifndef CONTENT_H
#define CONTENT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ContentProvider ContentProvider;
typedef struct Logger Logger;

/// @brief A chunk of text content returned by a ContentProvider.
typedef struct ContentChunk {
    char text[4096]; ///< Chunk text content.
    size_t length;   ///< Text length in bytes (excluding null terminator).
} ContentChunk;

/// @brief Content selection mode for providers.
typedef enum ContentMode {
    CONTENT_MODE_SENTENCES,    ///< Full sentences (DB: typing_sentences; File: lines)
    CONTENT_MODE_COMMON_WORDS, ///< Words by frequency (DB only; File: unsupported)
    CONTENT_MODE_RANDOM_WORDS  ///< Words in random order (DB: ORDER BY RANDOM; File: shuffle)
} ContentMode;

// ── Factory Functions ────────────────────────────────────────────────────────

/// @brief Create a ContentProvider from a string.
/// @param text Source text (copied internally).
/// @return New ContentProvider, or NULL on allocation failure.
ContentProvider* contentProviderFromString(const char* text);

/// @brief Create a ContentProvider reading from a file.
/// @param filepath Path to text file (copied internally).
/// @return New ContentProvider, or NULL on allocation failure.
ContentProvider* contentProviderFromFile(const char* filepath);

/// @brief Create a ContentProvider backed by a SQLite database.
///        Default mode: CONTENT_MODE_COMMON_WORDS, limit 300.
/// @param filepath Path to SQLite content database (copied internally).
/// @return New ContentProvider, or NULL on allocation failure.
ContentProvider* contentProviderFromDatabase(const char* filepath);

/// @brief Create a ContentProvider fetching from a URL.
/// @param url URL to fetch from (copied internally).
/// @return New ContentProvider, or NULL on allocation failure.
/// @note Currently falls back to default string; web provider not implemented.
ContentProvider* contentProviderFromWeb(const char* url);

/// @brief Destroy a ContentProvider and free resources.
/// @param provider ContentProvider to destroy (NULL is safe).
void contentProviderDestroy(ContentProvider* provider);

// ── Logger ───────────────────────────────────────────────────────────────────

/// @brief Attach a Logger for diagnostic output.
/// @param self   ContentProvider instance.
/// @param logger Logger to attach (NULL to detach).
void contentProviderSetLogger(ContentProvider* self, Logger* logger);

// ── Database Provider Configuration ──────────────────────────────────────────

/// @brief Set content selection mode.
/// @param self ContentProvider instance.
/// @param mode ContentMode to use.
void contentProviderSetMode(ContentProvider* self, ContentMode mode);

/// @brief Set maximum rows to fetch from database.
/// @param self  ContentProvider instance.
/// @param limit Maximum rows per query (default 300).
void contentProviderSetContentLimit(ContentProvider* self, size_t limit);

// ── Content Access ───────────────────────────────────────────────────────────

/// @brief Get the next content chunk.
/// @param provider ContentProvider instance.
/// @return ContentChunk with next content, or empty chunk if NULL/exhausted.
ContentChunk contentProviderGetNext(ContentProvider* provider);

/// @brief Reset provider to beginning of content.
/// @param provider ContentProvider instance (NULL is safe).
void contentProviderReset(ContentProvider* provider);

/// @brief Check if provider has exhausted its content.
/// @param provider ContentProvider instance.
/// @return true if all content consumed or provider is NULL.
bool contentProviderIsExhausted(ContentProvider* provider);

#ifdef __cplusplus
}
#endif

#endif // CONTENT_H