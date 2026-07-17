#include "formatter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct Formatter {
    char buffer[4096];
    size_t position;
} Formatter;

Formatter* formatterCreate(void) {
    Formatter* f = malloc(sizeof(Formatter));
    if (!f) return NULL;
    f->buffer[0] = '\0';
    f->position = 0;
    return f;
}

void formatterDestroy(Formatter* formatter) {
    if (formatter) free(formatter);
}

void formatterReset(Formatter* formatter) {
    if (!formatter) return;
    formatter->buffer[0] = '\0';
    formatter->position = 0;
}

ContentChunk formatterFormat(Formatter* formatter, const char* text, size_t maxChunkSize) {
    ContentChunk chunk;
    memset(&chunk, 0, sizeof(chunk));
    
    if (!formatter || !text || maxChunkSize == 0) return chunk;
    
    size_t textLen = strlen(text);
    if (textLen == 0) return chunk;
    
    // If remaining text fits in one chunk, return it all
    if (textLen <= maxChunkSize) {
        snprintf(chunk.text, sizeof(chunk.text), "%s", text);
        chunk.length = textLen;
        return chunk;
    }
    
    // Find a good break point: prefer sentence boundaries (., !, ?)
    size_t breakPoint = maxChunkSize;
    for (size_t i = maxChunkSize / 2; i < maxChunkSize; i++) {
        if (text[i] == '.' || text[i] == '!' || text[i] == '?') {
            // Include the punctuation and any following whitespace
            breakPoint = i + 1;
            while (breakPoint < textLen && isspace((unsigned char)text[breakPoint])) {
                breakPoint++;
            }
            break;
        }
    }
    
    // If no sentence boundary found, break at last space before maxChunkSize
    if (breakPoint == maxChunkSize) {
        for (size_t i = maxChunkSize - 1; i > maxChunkSize / 2; i--) {
            if (isspace((unsigned char)text[i])) {
                breakPoint = i;
                // Skip leading whitespace of next chunk
                while (breakPoint < textLen && isspace((unsigned char)text[breakPoint])) {
                    breakPoint++;
                }
                break;
            }
        }
    }
    
    // Copy the chunk
    snprintf(chunk.text, sizeof(chunk.text), "%.*s", (int)breakPoint, text);
    chunk.length = breakPoint;
    
    return chunk;
}