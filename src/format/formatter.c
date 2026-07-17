#include "formatter.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct Formatter {
    char buffer[4096];
    size_t position;
    Logger* logger;
} Formatter;

Formatter* formatterCreate(void) {
    Formatter* f = malloc(sizeof(Formatter));
    if (!f) {
        fprintf(stderr, "[ERROR] Failed to allocate Formatter\n");
        return NULL;
    }
    f->buffer[0] = '\0';
    f->position = 0;
    f->logger = NULL;
    return f;
}

void formatterDestroy(Formatter* formatter) {
    if (formatter) {
        if (formatter->logger) loggerLog(formatter->logger, LOG_LEVEL_DEBUG, "Formatter destroyed");
        free(formatter);
    }
}

void formatterSetLogger(Formatter* self, Logger* logger) {
    if (self) self->logger = logger;
}

void formatterReset(Formatter* formatter) {
    if (!formatter) return;
    formatter->buffer[0] = '\0';
    formatter->position = 0;
}

ContentChunk formatterFormat(Formatter* formatter, const char* text, size_t maxChunkSize) {
    ContentChunk chunk;
    memset(&chunk, 0, sizeof(chunk));
    
    if (!formatter || !text || maxChunkSize == 0) {
        if (formatter && formatter->logger) loggerLog(formatter->logger, LOG_LEVEL_WARNING, "Formatter: invalid input");
        return chunk;
    }
    
    size_t textLen = strlen(text);
    if (textLen == 0) return chunk;
    
    if (textLen <= maxChunkSize) {
        snprintf(chunk.text, sizeof(chunk.text), "%s", text);
        chunk.length = textLen;
        if (formatter->logger) loggerLog(formatter->logger, LOG_LEVEL_DEBUG, "Formatter: text fits in one chunk");
        return chunk;
    }
    
    size_t breakPoint = maxChunkSize;
    for (size_t i = maxChunkSize / 2; i < maxChunkSize; i++) {
        if (text[i] == '.' || text[i] == '!' || text[i] == '?') {
            breakPoint = i + 1;
            while (breakPoint < textLen && isspace((unsigned char)text[breakPoint])) {
                breakPoint++;
            }
            break;
        }
    }
    
    if (breakPoint == maxChunkSize) {
        for (size_t i = maxChunkSize - 1; i > maxChunkSize / 2; i--) {
            if (isspace((unsigned char)text[i])) {
                breakPoint = i;
                while (breakPoint < textLen && isspace((unsigned char)text[breakPoint])) {
                    breakPoint++;
                }
                break;
            }
        }
    }
    
    snprintf(chunk.text, sizeof(chunk.text), "%.*s", (int)breakPoint, text);
    chunk.length = breakPoint;
    
    if (formatter->logger) loggerLog(formatter->logger, LOG_LEVEL_DEBUG, "Formatter: chunk created");
    
    return chunk;
}