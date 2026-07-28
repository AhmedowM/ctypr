"""Analyze cleaned sentence JSONL and append computed fields."""

import json
import sys


HOME_ROW = set("asdfghjkl;")
RARE_LETTERS_SCORE = {"z": 2, "q": 2, "x": 1.5, "j": 1, "v": 1, "k": 1}
PUNCTUATION = set(".,;:!?\"'")


def analyze(text):
    char_count = len(text)
    words = text.split()
    word_count = len(words)
    avg_word_length = char_count / word_count if word_count > 0 else 0.0

    lower = text.lower()
    unique_chars = set(lower)
    unique_char_ratio = len(unique_chars) / char_count if char_count > 0 else 0.0

    rare_score = sum(RARE_LETTERS_SCORE.get(c, 0) for c in lower)

    has_punctuation = any(ch in PUNCTUATION for ch in text)

    has_numbers = any(ch.isdigit() for ch in text)

    # Mixed case: has any lowercase AND any uppercase beyond first letter
    has_lower = any(ch.islower() for ch in text)
    has_upper_beyond_first = any(ch.isupper() for ch in text[1:]) if len(text) > 1 else False
    has_mixed_case = has_lower and has_upper_beyond_first

    home_row_chars = sum(1 for ch in lower if ch in HOME_ROW)
    home_row_ratio = home_row_chars / char_count if char_count > 0 else 0.0

    return {
        "char_count": char_count,
        "word_count": word_count,
        "avg_word_length": round(avg_word_length, 2),
        "unique_char_ratio": round(unique_char_ratio, 3),
        "rare_letter_score": rare_score,
        "has_punctuation": has_punctuation,
        "has_numbers": has_numbers,
        "has_mixed_case": has_mixed_case,
        "home_row_ratio": round(home_row_ratio, 3),
    }


def main():
    total = 0
    for filepath in sys.argv[1:] or ["-"]:
        f = open(filepath, "r", encoding="utf-8") if filepath != "-" else sys.stdin
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                record = json.loads(line)
            except json.JSONDecodeError:
                continue

            text = record.get("text", "")
            if not text:
                continue

            analysis = analyze(text)
            record.update(analysis)
            sys.stdout.write(json.dumps(record) + "\n")
            total += 1

        if f is not sys.stdin:
            f.close()

    print(f"[INFO] Analyze: {total} records analyzed", file=sys.stderr)


if __name__ == "__main__":
    main()
