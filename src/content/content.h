#ifndef CONTENT_H
#define CONTENT_H

#include <stdint.h>
#include <stdbool.h>

#include <stddef.h>

typedef struct ContentProvider ContentProvider;
typedef struct Logger Logger;

/// @brief A chunk of text content returned by a ContentProvider.
typedef struct ContentChunk {
    char text[4096]; ///< The chunk text content.
    size_t length;   ///< Length of the text in bytes (excluding null terminator).
} ContentChunk;

/// @brief Query mode for the database-backed content provider.
typedef enum ContentDbMode {
    CONTENT_DB_COMMON_WORDS, ///< Select words ordered by frequency rank
    CONTENT_DB_RANDOM_WORDS, ///< Select words in random order
    CONTENT_DB_SENTENCES     ///< Select sentences
} ContentDbMode;

// ── Factory Functions ────────────────────────────────────────────────────────

/// @brief Create a ContentProvider from a literal string.
/// @param text The source text (copied internally).
/// @return A new ContentProvider, or NULL on allocation failure.
ContentProvider* contentProviderFromString(const char* text);

/// @brief Create a ContentProvider that reads from a file.
/// @param filepath Path to the text file (copied internally).
/// @return A new ContentProvider, or NULL on allocation failure.
ContentProvider* contentProviderFromFile(const char* filepath);

/// @brief Create a ContentProvider backed by a database query.
///        Default mode is CONTENT_DB_COMMON_WORDS with limit 300.
/// @param filepath Path to the SQLite content database file (copied internally).
/// @return A new ContentProvider, or NULL on allocation failure.
ContentProvider* contentProviderFromDatabase(const char* filepath);

/// @brief Create a ContentProvider that fetches content from a URL.
/// @param url The URL to fetch from (copied internally).
/// @return A new ContentProvider, or NULL on allocation failure.
/// @note Currently falls back to a default example string; web provider is not yet implemented.
ContentProvider* contentProviderFromWeb(const char* url);

/// @brief Destroy a ContentProvider and free all associated resources.
/// @param provider The ContentProvider to destroy (NULL is safe).
void contentProviderDestroy(ContentProvider* provider);

// ── Logger ───────────────────────────────────────────────────────────────────

/// @brief Attach a Logger to the content provider for diagnostic output.
/// @param self   The ContentProvider instance.
/// @param logger The Logger instance to attach (NULL to detach).
void contentProviderSetLogger(ContentProvider* self, Logger* logger);

// ── Database Provider Configuration ──────────────────────────────────────────

/// @brief Set the query mode for a database-backed content provider.
/// @param self The ContentProvider instance.
/// @param mode The ContentDbMode to use.
void contentProviderSetDbMode(ContentProvider* self, ContentDbMode mode);

/// @brief Set the maximum number of rows to fetch from the database.
/// @param self  The ContentProvider instance.
/// @param limit Maximum rows per query (default 300).
void contentProviderSetContentLimit(ContentProvider* self, size_t limit);

// ── Content Access ───────────────────────────────────────────────────────────

/// @brief Get the next chunk of content from the provider.
/// @param provider The ContentProvider instance.
/// @return A ContentChunk containing the next piece of content.
///         Returns an empty chunk if provider is NULL or exhausted.
ContentChunk contentProviderGetNext(ContentProvider* provider);

/// @brief Reset the provider to the beginning of its content.
/// @param provider The ContentProvider instance (NULL is safe).
void contentProviderReset(ContentProvider* provider);

/// @brief Check if the provider has exhausted its content.
/// @param provider The ContentProvider instance.
/// @return true if all content has been consumed or provider is NULL.
bool contentProviderIsExhausted(ContentProvider* provider);

#endif // CONTENT_H
