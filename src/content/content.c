#include "content.h"

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

    char* filepath; // For file-based providers
    char* url;      // For web-based providers
    ContentProviderType type;
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
    return cp;
}

void contentProviderDestroy(ContentProvider* provider) {
    if (provider) {
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
    
    if (!cp->filepath) return c;
    
    FILE* f = fopen(cp->filepath, "r");
    if (!f) return c;
    
    // Read entire file into text buffer
    size_t total = 0;
    size_t n;
    while (total < sizeof(cp->text) - 1 && (n = fread(cp->text + total, 1, sizeof(cp->text) - 1 - total, f)) > 0) {
        total += n;
    }
    cp->text[total] = '\0';
    cp->length = total;
    cp->currentIndex = 0;
    
    fclose(f);
    
    snprintf(c.text, sizeof(c.text), "%s", cp->text);
    c.length = cp->length;
    return c;
}

ContentChunk _cpGetDatabase(ContentProvider* cp) {
    ContentChunk c;
    memset(&c, 0, sizeof(c));
    
    // Placeholder: database provider not yet implemented
    // log(WARN, "Database ContentProvider is not implemented yet. Returning empty content.");
    
    return c;
}

ContentChunk _cpGetWeb(ContentProvider* cp) {
    ContentChunk c;
    memset(&c, 0, sizeof(c));
    
    // Placeholder: web provider not yet implemented
    // log(WARN, "Web ContentProvider is not implemented yet. Redirected to default example string provider.");
    
    // Fallback to default string
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