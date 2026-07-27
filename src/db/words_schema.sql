-- Words database for ctypr content provider
-- Database file: words.db
-- Used with: contentProviderFromDatabase("words.db") + CONTENT_MODE_COMMON_WORDS or CONTENT_MODE_RANDOM_WORDS
--
-- Built by: python build_db.py --words ... --words-db words.db

CREATE TABLE IF NOT EXISTS common_words (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    word TEXT NOT NULL UNIQUE,
    word_length INTEGER NOT NULL,
    frequency_rank INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS random_words (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    word TEXT NOT NULL UNIQUE,
    word_length INTEGER NOT NULL,
    difficulty_rating REAL DEFAULT 1.0
);
