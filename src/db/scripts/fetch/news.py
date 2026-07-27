"""Fetch sentences from RSS news feeds."""

import json
import os
import re
import sys
import time


FEEDS = [
    ("BBC News", "https://feeds.bbci.co.uk/news/rss.xml"),
    ("NPR", "https://www.npr.org/rss/rss.php"),
    ("The Economist", "https://www.economist.com/feeds/print-sections/77/business.xml"),
]

CACHE_DIR = ".feed_cache"
CACHE_MAX_AGE = 86400
MIN_SENTENCE_LEN = 20


def fetch_feed(url):
    import feedparser

    cache_key = re.sub(r"[^a-zA-Z0-9]", "_", url)
    cache_path = os.path.join(CACHE_DIR, cache_key)

    if os.path.exists(cache_path):
        age = time.time() - os.path.getmtime(cache_path)
        if age < CACHE_MAX_AGE:
            with open(cache_path, "r", encoding="utf-8") as f:
                return f.read()

    try:
        feed = feedparser.parse(url)
        raw = json.dumps(feed.entries)
    except Exception as e:
        print(f"[ERROR] Failed to parse feed {url}: {e}", file=sys.stderr)
        return "[]"

    os.makedirs(CACHE_DIR, exist_ok=True)
    with open(cache_path, "w", encoding="utf-8") as f:
        f.write(raw)

    return raw


def split_sentences(text):
    raw = re.split(r"\.\s+", text)
    result = []
    for s in raw:
        s = s.strip().rstrip(".")
        if len(s) >= MIN_SENTENCE_LEN:
            result.append(s)
    return result


def main():
    try:
        import feedparser
    except ImportError:
        print("[ERROR] feedparser is required. Install: pip install feedparser", file=sys.stderr)
        sys.exit(1)

    seen = set()
    count = 0

    for feed_name, feed_url in FEEDS:
        raw = fetch_feed(feed_url)
        try:
            entries = json.loads(raw) if raw.startswith("[") else []
        except (json.JSONDecodeError, TypeError):
            entries = []

        if not entries:
            try:
                feed = feedparser.parse(feed_url)
                entries = feed.entries
            except Exception as e:
                print(f"[ERROR] {feed_name}: {e}", file=sys.stderr)
                continue

        feed_count = 0
        for entry in entries:
            title = getattr(entry, "title", "") or ""
            desc = getattr(entry, "description", "") or ""

            texts = [title] + split_sentences(desc)
            for text in texts:
                text = text.strip()
                if not text:
                    continue
                lower = text.lower()
                if lower in seen:
                    continue
                seen.add(lower)

                record = {
                    "text": text,
                    "source": feed_name,
                    "title": feed_name,
                    "author": feed_name,
                }
                sys.stdout.write(json.dumps(record) + "\n")
                count += 1
                feed_count += 1

        print(f"[INFO] {feed_name}: {feed_count} items", file=sys.stderr)

    print(f"[INFO] News feeds: {count} total items", file=sys.stderr)


if __name__ == "__main__":
    main()
