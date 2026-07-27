"""Fetch and score word lists with real frequency data."""

import json
import os
import sys
import urllib.request
import urllib.error


WORDLIST_URL = "https://raw.githubusercontent.com/dwyl/english-words/master/words.txt"
FREQLIST_URL = "https://raw.githubusercontent.com/hermitdave/FrequencyWords/master/content/2016/en/en_full.txt"
SYSTEM_PATHS = ["/usr/share/dict/words", "/usr/dict/words", "/usr/share/dict/american-english"]
MIN_LEN = 3
MAX_LEN = 12
MAX_WORDS = 120000
RARE_LETTERS = {"z": 2, "q": 2, "x": 1.5, "j": 1, "v": 1, "k": 1}


def load_wordlist():
    for path in SYSTEM_PATHS:
        if os.path.exists(path):
            print(f"[INFO] Using system word list: {path}", file=sys.stderr)
            with open(path, "r", encoding="utf-8", errors="ignore") as f:
                return [line.strip() for line in f if line.strip()]
    print("[INFO] Downloading word list from GitHub...", file=sys.stderr)
    try:
        with urllib.request.urlopen(WORDLIST_URL, timeout=30) as resp:
            data = resp.read().decode("utf-8", errors="ignore")
        return data.splitlines()
    except (urllib.error.URLError, OSError) as e:
        print(f"[ERROR] Failed to download word list: {e}", file=sys.stderr)
        return []


def is_valid_word(word):
    if len(word) < MIN_LEN or len(word) > MAX_LEN:
        return False
    if "'" in word or "-" in word:
        return False
    if word[0].isupper():
        return False
    if not word.isascii() or not word.isalpha():
        return False
    return True


def word_difficulty_rating(word):
    rare = sum(RARE_LETTERS.get(c, 0) for c in word.lower())
    bonus = 1.0 if rare >= 2 else (0.5 if rare >= 1 else 0.0)
    return 1.0 + max(0, (len(word) - 4)) * 0.2 + bonus


def load_frequencies():
    try:
        import wordfreq
        print("[INFO] Using wordfreq for frequency data", file=sys.stderr)
        return ("wordfreq", wordfreq)
    except ImportError:
        pass

    print("[INFO] Downloading frequency list from GitHub...", file=sys.stderr)
    try:
        with urllib.request.urlopen(FREQLIST_URL, timeout=60) as resp:
            data = resp.read().decode("utf-8", errors="ignore")
        freqs = {}
        for line in data.splitlines():
            parts = line.strip().rsplit(" ", 1)
            if len(parts) == 2 and parts[0].isalpha() and parts[0].islower():
                freqs[parts[0]] = int(parts[1])
        print(f"[INFO] Frequency list: {len(freqs):,} entries", file=sys.stderr)
        return ("dict", freqs)
    except (urllib.error.URLError, OSError) as e:
        print(f"[WARN] Frequency download failed: {e}", file=sys.stderr)
        return ("none", None)


def lookup_freq(word, source):
    kind, data = source
    if kind == "wordfreq":
        return data.word_frequency(word, "en")
    elif kind == "dict":
        return data.get(word, 0)
    return 0


def main():
    raw = load_wordlist()
    if not raw:
        print("[ERROR] No word list available.", file=sys.stderr)
        sys.exit(1)

    source = load_frequencies()
    has_freq = source[0] != "none"

    entries = []
    for w in raw:
        w = w.strip()
        if not is_valid_word(w):
            continue
        freq = lookup_freq(w, source) if has_freq else 0
        entries.append((w, freq))

    if not entries:
        print("[ERROR] No valid words after filtering.", file=sys.stderr)
        sys.exit(1)

    # Sort: frequency desc, then word length asc, then alphabetical
    entries.sort(key=lambda e: (-e[1], len(e[0]), e[0]))

    entries = entries[:MAX_WORDS]

    for rank, (word, _freq) in enumerate(entries, start=1):
        record = {
            "word": word,
            "length": len(word),
            "difficulty_rating": round(word_difficulty_rating(word), 1),
            "frequency_rank": rank,
        }
        sys.stdout.write(json.dumps(record) + "\n")

    print(f"[INFO] Words: {len(entries):,} output (source: {source[0]})", file=sys.stderr)


if __name__ == "__main__":
    main()
