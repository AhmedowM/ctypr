#include "content.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* Simple test framework */
static int tests_passed = 0;
static int tests_failed = 0;
static int test_count = 0;

#define TEST(name) do { \
    test_count++; \
    printf("  TEST %d: %s ... ", test_count, name); \
    fflush(stdout); \
} while(0)

#define PASS() do { \
    tests_passed++; \
    printf("PASSED\n"); \
} while(0)

#define FAIL(msg) do { \
    tests_failed++; \
    printf("FAILED: %s (line %d)\n", msg, __LINE__); \
} while(0)

#define ASSERT(cond, msg) do { \
    if (!(cond)) { FAIL(msg); return; } \
} while(0)

/* ===== Test: String provider ===== */
static void test_string_provider(void) {
    TEST("String provider: create and get content");
    ContentProvider* cp = contentProviderFromString("Hello World");
    ASSERT(cp != NULL, "provider should not be NULL");
    
    ContentChunk chunk = contentProviderGetNext(cp);
    ASSERT(strcmp(chunk.text, "Hello World") == 0, "text should match");
    ASSERT(chunk.length == 11, "length should be 11");
    
    contentProviderDestroy(cp);
    PASS();
}

static void test_string_provider_empty(void) {
    TEST("String provider: empty string");
    ContentProvider* cp = contentProviderFromString("");
    ASSERT(cp != NULL, "provider should not be NULL");
    
    ContentChunk chunk = contentProviderGetNext(cp);
    ASSERT(chunk.length == 0, "length should be 0");
    ASSERT(chunk.text[0] == '\0', "text should be empty");
    
    contentProviderDestroy(cp);
    PASS();
}

static void test_string_provider_reset(void) {
    TEST("String provider: reset");
    ContentProvider* cp = contentProviderFromString("Test");
    ASSERT(cp != NULL, "provider should not be NULL");
    
    contentProviderReset(cp);
    ASSERT(!contentProviderIsExhausted(cp), "should not be exhausted after reset");
    
    contentProviderDestroy(cp);
    PASS();
}

/* ===== Test: File provider ===== */
static void test_file_provider(void) {
    TEST("File provider: create and read file");
    
    // Create a temporary test file
    FILE* f = fopen("test_content.txt", "w");
    if (!f) {
        printf("SKIPPED (cannot create test file)\n");
        return;
    }
    fprintf(f, "File content test\n");
    fclose(f);
    
    ContentProvider* cp = contentProviderFromFile("test_content.txt");
    ASSERT(cp != NULL, "provider should not be NULL");
    
    ContentChunk chunk = contentProviderGetNext(cp);
    ASSERT(strcmp(chunk.text, "File content test\n") == 0, "text should match file content");
    ASSERT(chunk.length == 18, "length should be 18");
    
    contentProviderDestroy(cp);
    remove("test_content.txt");
    PASS();
}

static void test_file_provider_missing(void) {
    TEST("File provider: missing file returns empty");
    ContentProvider* cp = contentProviderFromFile("nonexistent_file.txt");
    ASSERT(cp != NULL, "provider should not be NULL");
    
    ContentChunk chunk = contentProviderGetNext(cp);
    ASSERT(chunk.length == 0, "length should be 0 for missing file");
    
    contentProviderDestroy(cp);
    PASS();
}

/* ===== Test: Web provider ===== */
static void test_web_provider(void) {
    TEST("Web provider: fallback to default string");
    ContentProvider* cp = contentProviderFromWeb("http://example.com");
    ASSERT(cp != NULL, "provider should not be NULL");
    
    ContentChunk chunk = contentProviderGetNext(cp);
    ASSERT(strcmp(chunk.text, "The quick brown fox jumps over the lazy dog.") == 0, 
           "web provider should fallback to default string");
    
    contentProviderDestroy(cp);
    PASS();
}

/* ===== Test: Database provider ===== */
static void test_database_provider(void) {
    TEST("Database provider: returns empty (not implemented)");
    ContentProvider* cp = contentProviderFromDatabase("test.db");
    ASSERT(cp != NULL, "provider should not be NULL");
    
    ContentChunk chunk = contentProviderGetNext(cp);
    ASSERT(chunk.length == 0, "database provider should return empty content");
    
    contentProviderDestroy(cp);
    PASS();
}

/* ===== Test: Provider lifecycle ===== */
static void test_provider_null_safety(void) {
    TEST("Provider: NULL safety");
    
    ContentChunk chunk = contentProviderGetNext(NULL);
    ASSERT(chunk.length == 0, "NULL provider should return empty chunk");
    
    contentProviderReset(NULL);
    ASSERT(contentProviderIsExhausted(NULL), "NULL provider should be exhausted");
    
    contentProviderDestroy(NULL);
    PASS();
}

static void test_provider_reset(void) {
    TEST("Provider: reset clears state");
    ContentProvider* cp = contentProviderFromString("Reset test");
    ASSERT(cp != NULL, "provider should not be NULL");
    
    // Get content
    ContentChunk chunk1 = contentProviderGetNext(cp);
    ASSERT(chunk1.length > 0, "should have content");
    
    // Reset
    contentProviderReset(cp);
    
    // Get again
    ContentChunk chunk2 = contentProviderGetNext(cp);
    ASSERT(chunk2.length == chunk1.length, "length should match after reset");
    ASSERT(strcmp(chunk2.text, chunk1.text) == 0, "text should match after reset");
    
    contentProviderDestroy(cp);
    PASS();
}

/* ===== Main ===== */
int main(void) {
    printf("=== ctypr Content Provider Test Suite ===\n\n");
    
    // String provider
    test_string_provider();
    test_string_provider_empty();
    test_string_provider_reset();
    
    // File provider
    test_file_provider();
    test_file_provider_missing();
    
    // Web provider
    test_web_provider();
    
    // Database provider
    test_database_provider();
    
    // Lifecycle
    test_provider_null_safety();
    test_provider_reset();
    
    printf("\n=== Results: %d passed, %d failed, %d total ===\n",
           tests_passed, tests_failed, test_count);
    
    return tests_failed > 0 ? 1 : 0;
}