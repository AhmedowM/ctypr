#ifndef FORMATTER_H
#define FORMATTER_H

#include <stdint.h>
#include <stdbool.h>
#include "content.h"

typedef struct Formatter Formatter;
typedef struct Logger Logger;

// ── Lifecycle ────────────────────────────────────────────────────────────────

/// @brief Create a new Formatter for text chunking.
/// @return New Formatter, or NULL on allocation failure.
Formatter* formatterCreate(void);

/// @brief Destroy a Formatter.
/// @param formatter Formatter to destroy (NULL is safe).
void formatterDestroy(Formatter* formatter);

// ── Logger ───────────────────────────────────────────────────────────────────

/// @brief Attach a Logger for diagnostic output.
/// @param self   Formatter instance.
/// @param logger Logger to attach (NULL to detach).
void formatterSetLogger(Formatter* self, Logger* logger);

// ── Formatting ───────────────────────────────────────────────────────────────

/// @brief Chunk input text into a segment not exceeding maxChunkSize.
///        Breaks at sentence boundaries (. ! ?), falling back to whitespace.
/// @param formatter    Formatter instance.
/// @param text         Input text to format.
/// @param maxChunkSize Maximum characters per chunk.
/// @return ContentChunk with first logical segment, or empty chunk if
///         formatter is NULL, text is NULL/empty, or maxChunkSize is 0.
ContentChunk formatterFormat(Formatter* formatter, const char* text, size_t maxChunkSize);

/// @brief Reset formatter internal state.
/// @param formatter Formatter instance (NULL is safe).
void formatterReset(Formatter* formatter);

#endif // FORMATTER_H