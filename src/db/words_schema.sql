-- Words database for ctypr content provider
-- Database file: words.db
-- Used with: contentProviderFromDatabase("words.db")
--
-- CONTENT_MODE_COMMON_WORDS reads: SELECT word FROM words ORDER BY frequency_rank ASC LIMIT ?
-- CONTENT_MODE_RANDOM_WORDS reads:  SELECT word FROM words ORDER BY RANDOM() LIMIT ?
--
-- Built by: python build_db.py --words ... --words-db words.db

CREATE TABLE IF NOT EXISTS words (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    word TEXT NOT NULL UNIQUE,
    word_length INTEGER NOT NULL,
    frequency_rank INTEGER NOT NULL,
    difficulty_rating REAL DEFAULT 1.0
);
