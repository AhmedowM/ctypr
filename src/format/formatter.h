#ifndef FORMATTER_H
#define FORMATTER_H

#include <stdint.h>
#include <stdbool.h>
#include "content.h"

typedef struct Formatter Formatter;
typedef struct Logger Logger;

// ── Lifecycle ────────────────────────────────────────────────────────────────

/// @brief Create a new Formatter instance for chunking text.
/// @return A pointer to the newly created Formatter, or NULL on allocation failure.
Formatter* formatterCreate(void);

/// @brief Destroy a Formatter instance.
/// @param formatter The Formatter to destroy (NULL is safe).
void formatterDestroy(Formatter* formatter);

// ── Logger ───────────────────────────────────────────────────────────────────

/// @brief Attach a Logger to the formatter for diagnostic output.
/// @param self   The Formatter instance.
/// @param logger The Logger instance to attach (NULL to detach).
void formatterSetLogger(Formatter* self, Logger* logger);

// ── Formatting ───────────────────────────────────────────────────────────────

/// @brief Format/chunk input text into a single chunk not exceeding maxChunkSize.
///        Attempts to break at sentence boundaries (. ! ?), falling back to
///        whitespace boundaries if no sentence boundary is found within range.
/// @param formatter    The Formatter instance.
/// @param text         The input text to format.
/// @param maxChunkSize Maximum number of characters per chunk.
/// @return A ContentChunk containing the first logical segment of text.
///         Returns an empty chunk if formatter is NULL, text is NULL/empty,
///         or maxChunkSize is 0.
ContentChunk formatterFormat(Formatter* formatter, const char* text, size_t maxChunkSize);

/// @brief Reset the formatter's internal state.
/// @param formatter The Formatter instance (NULL is safe).
void formatterReset(Formatter* formatter);

#endif // FORMATTER_H
