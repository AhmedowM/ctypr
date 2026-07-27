"""Fetch and score word lists from system dictionaries or fallback."""

import json
import os
import random
import sys
import urllib.request
import urllib.error


FALLBACK_URL = "https://raw.githubusercontent.com/dwyl/english-words/master/words.txt"
SYSTEM_PATHS = ["/usr/share/dict/words", "/usr/dict/words", "/usr/share/dict/american-english"]
MIN_LEN = 3
MAX_LEN = 12
RARE_LETTERS = {"z": 2, "q": 2, "x": 1.5, "j": 1, "v": 1, "k": 1}


def find_wordlist():
    for path in SYSTEM_PATHS:
        if os.path.exists(path):
            return path
    return None


def download_wordlist():
    print("[INFO] Downloading word list from GitHub...", file=sys.stderr)
    try:
        with urllib.request.urlopen(FALLBACK_URL, timeout=30) as resp:
            data = resp.read().decode("utf-8", errors="ignore")
        return data.splitlines()
    except (urllib.error.URLError, OSError) as e:
        print(f"[ERROR] Failed to download word list: {e}", file=sys.stderr)
        return []


def load_words():
    path = find_wordlist()
    if path:
        print(f"[INFO] Using system word list: {path}", file=sys.stderr)
        with open(path, "r", encoding="utf-8", errors="ignore") as f:
            return [line.strip() for line in f if line.strip()]

    return download_wordlist()


def is_valid_word(word):
    if len(word) < MIN_LEN or len(word) > MAX_LEN:
        return False
    if "'" in word or "-" in word:
        return False
    if word[0].isupper():
        return False
    if not word.isascii():
        return False
    if not word.isalpha():
        return False
    return True


def word_difficulty_rating(word):
    rare = sum(RARE_LETTERS.get(c, 0) for c in word.lower())
    bonus = 0.0
    if rare >= 2:
        bonus = 1.0
    elif rare >= 1:
        bonus = 0.5
    return 1.0 + max(0, (len(word) - 4)) * 0.2 + bonus


def word_frequency_rank(word):
    return len(word) * 1000 + random.randint(0, 999)


def main():
    words = load_words()
    if not words:
        print("[ERROR] No word list available.", file=sys.stderr)
        sys.exit(1)

    count = 0
    for word in words:
        word = word.strip()
        if not is_valid_word(word):
            continue

        record = {
            "word": word,
            "length": len(word),
            "difficulty_rating": round(word_difficulty_rating(word), 1),
            "frequency_rank": word_frequency_rank(word),
        }
        sys.stdout.write(json.dumps(record) + "\n")
        count += 1

    print(f"[INFO] Words: {count} valid words output", file=sys.stderr)


if __name__ == "__main__":
    main()
