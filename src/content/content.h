#ifndef CONTENT_H
#define CONTENT_H

#include <stdint.h>
#include <stdbool.h>

typedef struct ContentProvider ContentProvider;
typedef struct Logger Logger;

/// @brief A chunk of text content returned by a ContentProvider.
typedef struct ContentChunk {
    char text[4096]; ///< The chunk text content.
    size_t length;   ///< Length of the text in bytes (excluding null terminator).
} ContentChunk;

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
/// @param filepath Path to the SQLite database file (copied internally).
/// @return A new ContentProvider, or NULL on allocation failure.
/// @note Currently returns empty content; database provider is not yet implemented.
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
