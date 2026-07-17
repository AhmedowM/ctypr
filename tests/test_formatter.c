#include "formatter.h"

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

/* ===== Test: Formatter basic functionality ===== */
static void test_formatter_short_text(void) {
    TEST("Formatter: short text fits in one chunk");
    Formatter* f = formatterCreate();
    ASSERT(f != NULL, "formatter should not be NULL");
    
    ContentChunk chunk = formatterFormat(f, "Hello", 100);
    ASSERT(strcmp(chunk.text, "Hello") == 0, "text should match");
    ASSERT(chunk.length == 5, "length should be 5");
    
    formatterDestroy(f);
    PASS();
}

static void test_formatter_empty_text(void) {
    TEST("Formatter: empty text returns empty chunk");
    Formatter* f = formatterCreate();
    ASSERT(f != NULL, "formatter should not be NULL");
    
    ContentChunk chunk = formatterFormat(f, "", 100);
    ASSERT(chunk.length == 0, "length should be 0");
    ASSERT(chunk.text[0] == '\0', "text should be empty");
    
    formatterDestroy(f);
    PASS();
}

static void test_formatter_sentence_boundary(void) {
    TEST("Formatter: splits at sentence boundary");
    Formatter* f = formatterCreate();
    ASSERT(f != NULL, "formatter should not be NULL");
    
    const char* text = "First sentence. Second sentence. Third sentence.";
    ContentChunk chunk = formatterFormat(f, text, 20);
    
    ASSERT(chunk.length > 0, "chunk should have content");
    ASSERT(strstr(chunk.text, "First sentence.") != NULL, "should include first sentence");
    
    formatterDestroy(f);
    PASS();
}

static void test_formatter_space_fallback(void) {
    TEST("Formatter: falls back to space when no sentence boundary");
    Formatter* f = formatterCreate();
    ASSERT(f != NULL, "formatter should not be NULL");
    
    const char* text = "This is a long text without punctuation that needs splitting";
    ContentChunk chunk = formatterFormat(f, text, 20);
    
    ASSERT(chunk.length > 0, "chunk should have content");
    ASSERT(chunk.length <= 20, "chunk should not exceed maxChunkSize");
    
    formatterDestroy(f);
    PASS();
}

static void test_formatter_reset(void) {
    TEST("Formatter: reset clears state");
    Formatter* f = formatterCreate();
    ASSERT(f != NULL, "formatter should not be NULL");
    
    formatterFormat(f, "Some text", 10);
    formatterReset(f);
    
    // After reset, formatting should work normally
    ContentChunk chunk = formatterFormat(f, "New text", 10);
    ASSERT(strcmp(chunk.text, "New text") == 0, "text should match after reset");
    
    formatterDestroy(f);
    PASS();
}

static void test_formatter_null_safety(void) {
    TEST("Formatter: NULL safety");
    
    ContentChunk chunk = formatterFormat(NULL, "text", 10);
    ASSERT(chunk.length == 0, "NULL formatter should return empty chunk");
    
    formatterReset(NULL);
    formatterDestroy(NULL);
    
    PASS();
}

static void test_formatter_null_text(void) {
    TEST("Formatter: NULL text returns empty chunk");
    Formatter* f = formatterCreate();
    ASSERT(f != NULL, "formatter should not be NULL");

    ContentChunk chunk = formatterFormat(f, NULL, 100);
    ASSERT(chunk.length == 0, "length should be 0");
    ASSERT(chunk.text[0] == '\0', "text should be empty");

    formatterDestroy(f);
    PASS();
}

static void test_formatter_zero_max_chunk(void) {
    TEST("Formatter: zero maxChunkSize returns empty");
    Formatter* f = formatterCreate();
    ASSERT(f != NULL, "formatter should not be NULL");
    
    ContentChunk chunk = formatterFormat(f, "Some text", 0);
    ASSERT(chunk.length == 0, "length should be 0");
    
    formatterDestroy(f);
    PASS();
}

/* ===== Main ===== */
int main(void) {
    printf("=== ctypr Formatter Test Suite ===\n\n");
    
    test_formatter_short_text();
    test_formatter_empty_text();
    test_formatter_sentence_boundary();
    test_formatter_space_fallback();
    test_formatter_reset();
    test_formatter_null_safety();
    test_formatter_null_text();
    test_formatter_zero_max_chunk();
    
    printf("\n=== Results: %d passed, %d failed, %d total ===\n",
           tests_passed, tests_failed, test_count);
    
    return tests_failed > 0 ? 1 : 0;
}