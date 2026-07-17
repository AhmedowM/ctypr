#ifndef FORMATTER_H
#define FORMATTER_H

#include <stdint.h>
#include <stdbool.h>
#include "content.h"

typedef struct Formatter Formatter;

Formatter* formatterCreate(void);
void formatterDestroy(Formatter* formatter);

ContentChunk formatterFormat(Formatter* formatter, const char* text, size_t maxChunkSize);
void formatterReset(Formatter* formatter);

#endif // FORMATTER_H