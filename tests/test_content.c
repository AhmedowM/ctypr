#include "content.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sqlite3.h>

#ifdef _WIN32
#include <windows.h>
#endif

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

static const char* TEST_DB = "test_content_words.db";
static const char* TEST_SENTENCE_DB = "test_content_sentences.db";

static int execSql(sqlite3* db, const char* sql) {
    char* err = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err);
    if (rc != SQLITE_OK && err) {
        printf("SQL error: %s\n", err);
        sqlite3_free(err);
    }
    return rc;
}

static void createTestWordDb(void) {
    remove(TEST_DB);
    sqlite3* db = NULL;
    ASSERT(sqlite3_open(TEST_DB, &db) == SQLITE_OK, "open test db");
    ASSERT(execSql(db, "CREATE TABLE IF NOT EXISTS common_words ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, word TEXT NOT NULL UNIQUE,"
        "word_length INTEGER NOT NULL, frequency_rank INTEGER NOT NULL)") == SQLITE_OK,
        "create common_words");
    ASSERT(execSql(db, "CREATE TABLE IF NOT EXISTS random_words ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, word TEXT NOT NULL UNIQUE,"
        "word_length INTEGER NOT NULL, difficulty_rating REAL DEFAULT 1.0)") == SQLITE_OK,
        "create random_words");
    ASSERT(execSql(db, "INSERT INTO common_words (word, word_length, frequency_rank) VALUES ('the', 3, 1)") == SQLITE_OK, "insert the");
    ASSERT(execSql(db, "INSERT INTO common_words (word, word_length, frequency_rank) VALUES ('quick', 5, 2)") == SQLITE_OK, "insert quick");
    ASSERT(execSql(db, "INSERT INTO common_words (word, word_length, frequency_rank) VALUES ('brown', 5, 3)") == SQLITE_OK, "insert brown");
    ASSERT(execSql(db, "INSERT INTO common_words (word, word_length, frequency_rank) VALUES ('fox', 3, 4)") == SQLITE_OK, "insert fox");
    ASSERT(execSql(db, "INSERT INTO random_words (word, word_length) VALUES ('jumps', 5)") == SQLITE_OK, "insert jumps");
    ASSERT(execSql(db, "INSERT INTO random_words (word, word_length) VALUES ('over', 4)") == SQLITE_OK, "insert over");
    ASSERT(execSql(db, "INSERT INTO random_words (word, word_length) VALUES ('lazy', 4)") == SQLITE_OK, "insert lazy");
    ASSERT(execSql(db, "INSERT INTO random_words (word, word_length) VALUES ('dog', 3)") == SQLITE_OK, "insert dog");
    sqlite3_close(db);
}

static void createTestSentenceDb(void) {
    remove(TEST_SENTENCE_DB);
    sqlite3* db = NULL;
    ASSERT(sqlite3_open(TEST_SENTENCE_DB, &db) == SQLITE_OK, "open sentence db");
    ASSERT(execSql(db, "CREATE TABLE IF NOT EXISTS typing_sentences ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, text_content TEXT NOT NULL,"
        "char_count INTEGER NOT NULL, word_count INTEGER NOT NULL,"
        "source_title TEXT NOT NULL, source_author TEXT DEFAULT 'Unknown',"
        "difficulty_category TEXT DEFAULT 'Normal')") == SQLITE_OK,
        "create typing_sentences");
    ASSERT(execSql(db, "INSERT INTO typing_sentences (text_content, char_count, word_count, source_title) "
        "VALUES ('The quick brown fox.', 20, 4, 'Test')") == SQLITE_OK, "insert sentence 1");
    ASSERT(execSql(db, "INSERT INTO typing_sentences (text_content, char_count, word_count, source_title) "
        "VALUES ('Jumps over the lazy dog.', 24, 5, 'Test')") == SQLITE_OK, "insert sentence 2");
    sqlite3_close(db);
}

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

/* ===== Test: Database provider - common words ===== */
static void test_database_common_words(void) {
    TEST("Database provider: common words mode");
    createTestWordDb();

    ContentProvider* cp = contentProviderFromDatabase(TEST_DB);
    ASSERT(cp != NULL, "provider should not be NULL");
    contentProviderSetDbMode(cp, CONTENT_DB_COMMON_WORDS);
    contentProviderSetContentLimit(cp, 10);

    ContentChunk chunk = contentProviderGetNext(cp);
    ASSERT(chunk.length > 0, "should have content");
    ASSERT(strstr(chunk.text, "the") != NULL, "should contain 'the'");
    ASSERT(strstr(chunk.text, "quick") != NULL, "should contain 'quick'");

    contentProviderDestroy(cp);
    remove(TEST_DB);
    PASS();
}

/* ===== Test: Database provider - random words ===== */
static void test_database_random_words(void) {
    TEST("Database provider: random words mode");
    createTestWordDb();

    ContentProvider* cp = contentProviderFromDatabase(TEST_DB);
    ASSERT(cp != NULL, "provider should not be NULL");
    contentProviderSetDbMode(cp, CONTENT_DB_RANDOM_WORDS);
    contentProviderSetContentLimit(cp, 10);

    ContentChunk chunk = contentProviderGetNext(cp);
    ASSERT(chunk.length > 0, "should have content");
    ASSERT(strstr(chunk.text, "jumps") != NULL, "should contain 'jumps'");

    contentProviderDestroy(cp);
    remove(TEST_DB);
    PASS();
}

/* ===== Test: Database provider - sentences ===== */
static void test_database_sentences(void) {
    TEST("Database provider: sentences mode");
    createTestSentenceDb();

    ContentProvider* cp = contentProviderFromDatabase(TEST_SENTENCE_DB);
    ASSERT(cp != NULL, "provider should not be NULL");
    contentProviderSetDbMode(cp, CONTENT_DB_SENTENCES);
    contentProviderSetContentLimit(cp, 10);

    ContentChunk chunk = contentProviderGetNext(cp);
    ASSERT(chunk.length > 0, "should have content");
    ASSERT(strstr(chunk.text, "quick brown fox") != NULL, "should contain sentence text");

    contentProviderDestroy(cp);
    remove(TEST_SENTENCE_DB);
    PASS();
}

/* ===== Test: Database provider - empty tables ===== */
static void test_database_empty_table(void) {
    TEST("Database provider: empty table returns empty");
    remove(TEST_DB);

    sqlite3* db = NULL;
    ASSERT(sqlite3_open(TEST_DB, &db) == SQLITE_OK, "open db");
    ASSERT(execSql(db, "CREATE TABLE common_words ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, word TEXT NOT NULL UNIQUE,"
        "word_length INTEGER NOT NULL, frequency_rank INTEGER NOT NULL)") == SQLITE_OK,
        "create table");
    sqlite3_close(db);

    ContentProvider* cp = contentProviderFromDatabase(TEST_DB);
    ASSERT(cp != NULL, "provider should not be NULL");

    ContentChunk chunk = contentProviderGetNext(cp);
    ASSERT(chunk.length == 0, "should be empty for empty table");

    contentProviderDestroy(cp);
    remove(TEST_DB);
    PASS();
}

/* ===== Test: Database provider - missing file ===== */
static void test_database_missing_file(void) {
    TEST("Database provider: missing file returns empty");
    remove("nonexistent_test.db");

    ContentProvider* cp = contentProviderFromDatabase("nonexistent_test.db");
    ASSERT(cp != NULL, "provider should not be NULL");

    ContentChunk chunk = contentProviderGetNext(cp);
    ASSERT(chunk.length == 0, "should be empty for missing file");

    contentProviderDestroy(cp);
    PASS();
}

/* ===== Test: Database provider - reset ===== */
static void test_database_reset(void) {
    TEST("Database provider: reset allows re-fetch");
    createTestWordDb();

    ContentProvider* cp = contentProviderFromDatabase(TEST_DB);
    ASSERT(cp != NULL, "provider should not be NULL");
    contentProviderSetDbMode(cp, CONTENT_DB_COMMON_WORDS);
    contentProviderSetContentLimit(cp, 10);

    ContentChunk chunk1 = contentProviderGetNext(cp);
    ASSERT(chunk1.length > 0, "first fetch should have content");

    ContentChunk chunk2 = contentProviderGetNext(cp);
    ASSERT(chunk2.length == 0, "second fetch should return empty (exhausted)");

    contentProviderReset(cp);
    ContentChunk chunk3 = contentProviderGetNext(cp);
    ASSERT(chunk3.length > 0, "after reset should have content again");
    ASSERT(strcmp(chunk3.text, chunk1.text) == 0, "content should match after reset");

    contentProviderDestroy(cp);
    remove(TEST_DB);
    PASS();
}

/* ===== Test: Database provider - content limit ===== */
static void test_database_content_limit(void) {
    TEST("Database provider: content limit respected");
    createTestWordDb();

    ContentProvider* cp = contentProviderFromDatabase(TEST_DB);
    ASSERT(cp != NULL, "provider should not be NULL");
    contentProviderSetDbMode(cp, CONTENT_DB_COMMON_WORDS);
    contentProviderSetContentLimit(cp, 2);

    ContentChunk chunk = contentProviderGetNext(cp);
    ASSERT(chunk.length > 0, "should have content");

    int wordCount = 0;
    for (size_t i = 0; i < chunk.length; i++) {
        if (chunk.text[i] == ' ') wordCount++;
    }
    wordCount++;
    ASSERT(wordCount <= 2, "should have at most 2 words");

    contentProviderDestroy(cp);
    remove(TEST_DB);
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

    ContentChunk chunk1 = contentProviderGetNext(cp);
    ASSERT(chunk1.length > 0, "should have content");

    contentProviderReset(cp);

    ContentChunk chunk2 = contentProviderGetNext(cp);
    ASSERT(chunk2.length == chunk1.length, "length should match after reset");
    ASSERT(strcmp(chunk2.text, chunk1.text) == 0, "text should match after reset");

    contentProviderDestroy(cp);
    PASS();
}

/* ===== Test: Database setter functions with defaults ===== */
static void test_database_default_mode(void) {
    TEST("Database provider: default mode is common words");
    ContentProvider* cp = contentProviderFromDatabase("test_default.db");
    ASSERT(cp != NULL, "provider should not be NULL");
    // Just verify it doesn't crash with defaults
    ContentChunk chunk = contentProviderGetNext(cp);
    ASSERT(chunk.length == 0, "empty DB means empty content");
    contentProviderDestroy(cp);
    remove("test_default.db");
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
    test_database_common_words();
    test_database_random_words();
    test_database_sentences();
    test_database_empty_table();
    test_database_missing_file();
    test_database_reset();
    test_database_content_limit();
    test_database_default_mode();

    // Lifecycle
    test_provider_null_safety();
    test_provider_reset();

    printf("\n=== Results: %d passed, %d failed, %d total ===\n",
           tests_passed, tests_failed, test_count);

    return tests_failed > 0 ? 1 : 0;
}
