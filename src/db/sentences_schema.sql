-- Sentences database for ctypr content provider
-- Database file: sentences.db
-- Used with: contentProviderFromDatabase("sentences.db") + CONTENT_MODE_SENTENCES
--
-- Built by: python build_db.py --sentences ... --sentences-db sentences.db

CREATE TABLE IF NOT EXISTS typing_sentences (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    text_content TEXT NOT NULL,
    char_count INTEGER NOT NULL,
    word_count INTEGER NOT NULL,
    source_title TEXT NOT NULL,
    source_author TEXT DEFAULT 'Unknown',
    difficulty_category TEXT DEFAULT 'Normal'
);
