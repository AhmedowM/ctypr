"""Extract sentences from local .txt files."""

import argparse
import glob as glob_module
import json
import os
import re
import sys


MIN_SENTENCE_LEN = 30


def is_binary(filepath):
    try:
        with open(filepath, "rb") as f:
            chunk = f.read(1024)
            return b"\0" in chunk
    except OSError:
        return True


def split_sentences(text):
    raw = re.split(r"\.\s+", text)
    result = []
    for s in raw:
        s = s.strip().rstrip(".")
        if len(s) >= MIN_SENTENCE_LEN:
            result.append(s)
    return result


def walk_files(paths, pattern):
    files = []
    for p in paths:
        if os.path.isfile(p):
            files.append(p)
        elif os.path.isdir(p):
            full_pattern = os.path.join(p, "**", pattern)
            matched = glob_module.glob(full_pattern, recursive=True)
            files.extend(f for f in matched if os.path.isfile(f))
        else:
            matched = glob_module.glob(p, recursive=True)
            files.extend(f for f in matched if os.path.isfile(f))
    return sorted(set(files))


def main():
    parser = argparse.ArgumentParser(description="Extract sentences from local text files")
    parser.add_argument("--path", "-p", action="append", default=[], help="File or directory path (repeatable)")
    parser.add_argument("--glob", "-g", default="*.txt", help="Glob pattern for files in directories (default: *.txt)")
    args = parser.parse_args()

    if not args.path:
        parser.print_help()
        sys.exit(1)

    files = walk_files(args.path, args.glob)
    if not files:
        print("[ERROR] No files found matching the given paths/patterns", file=sys.stderr)
        sys.exit(1)

    count = 0
    seen = set()

    for filepath in files:
        if is_binary(filepath):
            print(f"[SKIP] Binary file: {filepath}", file=sys.stderr)
            continue

        try:
            with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
                content = f.read()
        except OSError as e:
            print(f"[ERROR] {filepath}: {e}", file=sys.stderr)
            continue

        filename = os.path.basename(filepath)
        sentences = split_sentences(content)
        file_count = 0

        for sentence in sentences:
            lower = sentence.lower()
            if lower in seen:
                continue
            seen.add(lower)

            record = {
                "text": sentence,
                "source": filename,
                "title": filename,
                "author": "Local file",
            }
            sys.stdout.write(json.dumps(record) + "\n")
            count += 1
            file_count += 1

        print(f"[INFO] {filename}: {file_count} sentences", file=sys.stderr)

    print(f"[INFO] Files: {count} total sentences from {len(files)} files", file=sys.stderr)


if __name__ == "__main__":
    main()
