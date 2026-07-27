"""Build sentences.db and/or words.db from final JSONL files."""

import argparse
import json
import os
import sqlite3
import sys


def make_sentence_db(path):
    if os.path.exists(path):
        os.remove(path)
    db = sqlite3.connect(path)
    db.execute("""
        CREATE TABLE IF NOT EXISTS typing_sentences (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            text_content TEXT NOT NULL,
            char_count INTEGER NOT NULL,
            word_count INTEGER NOT NULL,
            source_title TEXT NOT NULL,
            source_author TEXT DEFAULT 'Unknown',
            difficulty_category TEXT DEFAULT 'Normal'
        )
    """)
    db.commit()
    return db


def make_word_db(path):
    if os.path.exists(path):
        os.remove(path)
    db = sqlite3.connect(path)
    db.execute("""
        CREATE TABLE IF NOT EXISTS common_words (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            word TEXT NOT NULL UNIQUE,
            word_length INTEGER NOT NULL,
            frequency_rank INTEGER NOT NULL
        )
    """)
    db.execute("""
        CREATE TABLE IF NOT EXISTS random_words (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            word TEXT NOT NULL UNIQUE,
            word_length INTEGER NOT NULL,
            difficulty_rating REAL DEFAULT 1.0
        )
    """)
    db.commit()
    return db


def insert_sentences(db, filepath):
    with open(filepath, "r", encoding="utf-8") as f:
        rows = [json.loads(line) for line in f if line.strip()]

    sql = """INSERT INTO typing_sentences
             (text_content, char_count, word_count, source_title, source_author, difficulty_category)
             VALUES (?, ?, ?, ?, ?, ?)"""

    count = 0
    for row in rows:
        try:
            db.execute(sql, (
                row["text"],
                row.get("char_count", len(row["text"])),
                row.get("word_count", len(row["text"].split())),
                row.get("source", row.get("title", "Unknown")),
                row.get("author", "Unknown"),
                row.get("difficulty_category", "Normal"),
            ))
            count += 1
        except (sqlite3.IntegrityError, KeyError) as e:
            print(f"[WARN] Skipping sentence: {e}", file=sys.stderr)

    db.commit()
    return count


def insert_words(db, filepath):
    with open(filepath, "r", encoding="utf-8") as f:
        rows = [json.loads(line) for line in f if line.strip()]

    rows.sort(key=lambda r: r.get("frequency_rank", 999999))

    common_count = 0
    random_count = 0

    for row in rows:
        word = row.get("word", "")
        word_len = row.get("length", len(word))
        freq = row.get("frequency_rank", 999999)
        diff = row.get("difficulty_rating", 1.0)

        try:
            db.execute(
                "INSERT INTO common_words (word, word_length, frequency_rank) VALUES (?, ?, ?)",
                (word, word_len, freq),
            )
            common_count += 1
        except sqlite3.IntegrityError:
            pass

        try:
            db.execute(
                "INSERT INTO random_words (word, word_length, difficulty_rating) VALUES (?, ?, ?)",
                (word, word_len, diff),
            )
            random_count += 1
        except sqlite3.IntegrityError:
            pass

    db.commit()
    return common_count, random_count


def main():
    parser = argparse.ArgumentParser(description="Build sentences.db and/or words.db from final JSONL")
    parser.add_argument("--sentences", help="Final sentences JSONL file")
    parser.add_argument("--words", help="Words JSONL file")
    parser.add_argument("--sentences-db", default="sentences.db", help="Output sentences database path")
    parser.add_argument("--words-db", default="words.db", help="Output words database path")
    args = parser.parse_args()

    if not args.sentences and not args.words:
        parser.error("At least one of --sentences or --words is required")

    if args.sentences:
        db = make_sentence_db(args.sentences_db)
        sent_count = insert_sentences(db, args.sentences)
        db.execute("VACUUM")
        db.close()
        size = os.path.getsize(args.sentences_db)
        print(f"[INFO] Built: {args.sentences_db}")
        print(f"  typing_sentences: {sent_count} rows")
        print(f"  file size:        {size:,} bytes ({size / 1024:.1f} KB)")

    if args.words:
        db = make_word_db(args.words_db)
        common_count, random_count = insert_words(db, args.words)
        db.execute("VACUUM")
        db.close()
        size = os.path.getsize(args.words_db)
        print(f"[INFO] Built: {args.words_db}")
        print(f"  common_words:     {common_count} rows")
        print(f"  random_words:     {random_count} rows")
        print(f"  file size:        {size:,} bytes ({size / 1024:.1f} KB)")


if __name__ == "__main__":
    main()
