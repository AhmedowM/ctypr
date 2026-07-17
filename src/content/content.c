#include "content.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
} ContentProvider;

ContentProvider* contentProviderFromString(const char* text) {
    ContentProvider* cp = malloc(sizeof(ContentProvider));
    if (!cp) return NULL;
    snprintf(cp->text, sizeof(cp->text), "%s", text);
    cp->length = strlen(cp->text);
    cp->currentIndex = 0;
    cp->type = CONTENT_PROVIDER_STRING;
    cp->filepath = NULL;
    cp->url = NULL;
    cp->logger = NULL;
    return cp;
}

ContentProvider* contentProviderFromFile(const char* filepath) {
    ContentProvider* cp = malloc(sizeof(ContentProvider));
    if (!cp) return NULL;
    
    cp->filepath = strdup(filepath);
    cp->text[0] = '\0';
    cp->length = 0;
    cp->currentIndex = 0;
    cp->type = CONTENT_PROVIDER_FILE;
    cp->url = NULL;
    cp->logger = NULL;
    return cp;
}

ContentProvider* contentProviderFromDatabase(const char* filepath) {
    ContentProvider* cp = malloc(sizeof(ContentProvider));
    if (!cp) return NULL;
    
    cp->filepath = strdup(filepath);
    cp->text[0] = '\0';
    cp->length = 0;
    cp->currentIndex = 0;
    cp->type = CONTENT_PROVIDER_DATABASE;
    cp->url = NULL;
    cp->logger = NULL;
    return cp;
}

ContentProvider* contentProviderFromWeb(const char* url) {
    ContentProvider* cp = malloc(sizeof(ContentProvider));
    if (!cp) return NULL;
    
    cp->url = strdup(url);
    snprintf(cp->text, sizeof(cp->text), "%s", "The quick brown fox jumps over the lazy dog.");
    cp->length = strlen(cp->text);
    cp->currentIndex = 0;
    cp->type = CONTENT_PROVIDER_WEB;
    cp->filepath = NULL;
    cp->logger = NULL;
    return cp;
}

void contentProviderSetLogger(ContentProvider* self, Logger* logger) {
    if (self) self->logger = logger;
}

void contentProviderDestroy(ContentProvider* provider) {
    if (provider) {
        if (provider->logger) loggerLog(provider->logger, LOG_LEVEL_DEBUG, "ContentProvider destroyed");
        free(provider->filepath);
        free(provider->url);
        free(provider);
    }
}

ContentChunk _cpGetString(ContentProvider* cp) {
    ContentChunk c;
    snprintf(c.text, sizeof(c.text), "%s", cp->text);
    c.length = strlen(c.text);
    return c;
}

ContentChunk _cpGetFile(ContentProvider* cp) {
    ContentChunk c;
    memset(&c, 0, sizeof(c));
    
    if (!cp->filepath) {
        if (cp->logger) loggerLog(cp->logger, LOG_LEVEL_WARNING, "File provider: no filepath set");
        return c;
    }
    
    FILE* f = fopen(cp->filepath, "r");
    if (!f) {
        if (cp->logger) loggerLog(cp->logger, LOG_LEVEL_WARNING, "File provider: could not open file");
        return c;
    }
    
    size_t total = 0;
    size_t n;
    while (total < sizeof(cp->text) - 1 && (n = fread(cp->text + total, 1, sizeof(cp->text) - 1 - total, f)) > 0) {
        total += n;
    }
    cp->text[total] = '\0';
    cp->length = total;
    cp->currentIndex = 0;
    
    fclose(f);
    
    if (cp->logger) loggerLog(cp->logger, LOG_LEVEL_DEBUG, "File provider: read file");
    
    snprintf(c.text, sizeof(c.text), "%s", cp->text);
    c.length = cp->length;
    return c;
}

ContentChunk _cpGetDatabase(ContentProvider* cp) {
    ContentChunk c;
    memset(&c, 0, sizeof(c));
    
    if (cp->logger) loggerLog(cp->logger, LOG_LEVEL_WARNING, "Database ContentProvider not implemented, returning empty");
    
    return c;
}

ContentChunk _cpGetWeb(ContentProvider* cp) {
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
}

bool contentProviderIsExhausted(ContentProvider* provider) {
    if (!provider) return true;
    return provider->currentIndex >= provider->length;
}