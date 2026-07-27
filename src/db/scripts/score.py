"""Assign difficulty scores to analyzed sentence/word JSONL."""

import json
import sys


def sentence_difficulty(row):
    score = 0.0
    if row.get("avg_word_length", 0) > 6:
        score += 1.0
    if row.get("has_punctuation", False):
        score += 0.5
    if row.get("has_numbers", False):
        score += 0.5
    if row.get("has_mixed_case", False):
        score += 0.5
    if row.get("rare_letter_score", 0) >= 3:
        score += 1.0
    if row.get("home_row_ratio", 1.0) < 0.3:
        score += 0.5
    if row.get("word_count", 0) > 25:
        score += 0.5

    if score < 1.0:
        return "Easy"
    elif score < 2.0:
        return "Normal"
    elif score < 3.0:
        return "Hard"
    else:
        return "Expert"


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

            if "word" in record:
                pass
            elif "text" in record:
                record["difficulty_category"] = sentence_difficulty(record)
            else:
                continue

            sys.stdout.write(json.dumps(record) + "\n")
            total += 1

        if f is not sys.stdin:
            f.close()

    print(f"[INFO] Score: {total} records scored", file=sys.stderr)


if __name__ == "__main__":
    main()
