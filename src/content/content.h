#ifndef CONTENT_H
#define CONTENT_H

#include <stdint.h>
#include <stdbool.h>

typedef struct ContentProvider ContentProvider;

typedef struct ContentChunk {
    char text[4096];
    size_t length;
} ContentChunk;

ContentProvider* contentProviderFromString(const char* text);
ContentProvider* contentProviderFromFile(const char* filepath);
ContentProvider* contentProviderFromDatabase(const char* filepath);
ContentProvider* contentProviderFromWeb(const char* url);
void contentProviderDestroy(ContentProvider* provider);

ContentChunk contentProviderGetNext(ContentProvider* provider);
void contentProviderReset(ContentProvider* provider);
bool contentProviderIsExhausted(ContentProvider* provider);

#endif // CONTENT_H