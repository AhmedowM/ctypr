"""Clean and filter JSONL sentence data."""

import json
import re
import sys


URL_RE = re.compile(r"https?://|www\.|@")
POETRY_START = re.compile(r"^[\s\-\*•\d\.]")


def is_valid(record):
    text = record.get("text", "")
    if not text:
        return False

    text = text.strip()
    if not text:
        return False

    if len(text) < 20 or len(text) > 500:
        return False

    for ch in text:
        if ord(ch) > 127:
            return False

    if URL_RE.search(text):
        return False

    if "\n\n" in text:
        return False

    if POETRY_START.match(text):
        return False

    return True


def main():
    seen = set()
    total = 0
    passed = 0

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

            total += 1

            if not is_valid(record):
                continue

            text = record["text"].strip()
            lower = text.lower()
            if lower in seen:
                continue
            seen.add(lower)

            record["text"] = text
            passed += 1
            sys.stdout.write(json.dumps(record) + "\n")

        if f is not sys.stdin:
            f.close()

    print(f"[INFO] Clean: {passed}/{total} passed", file=sys.stderr)


if __name__ == "__main__":
    main()
