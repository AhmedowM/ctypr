"""Fetch sentences from random Wikipedia articles."""

import json
import re
import sys
import time
import urllib.request
import urllib.error


API_URL = "https://en.wikipedia.org/w/api.php"
MAX_SENTENCES = 5000
BATCH_SIZE = 10
ITERATIONS = 500
MIN_SENTENCE_LEN = 30
DELAY = 1.0
RETRIES = 3
BASE_TIMEOUT = 30
USER_AGENT = "ctypr-content-builder/1.0"


def fetch_random_extracts(count):
    params = (
        f"action=query"
        f"&generator=random"
        f"&grnnamespace=0"
        f"&grnlimit={count}"
        f"&prop=extracts"
        f"&exintro=1"
        f"&explaintext=1"
        f"&format=json"
    )
    url = f"{API_URL}?{params}"
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    data = None
    for attempt in range(1, RETRIES + 1):
        try:
            with urllib.request.urlopen(req, timeout=BASE_TIMEOUT) as resp:
                data = json.loads(resp.read())
                break
        except (urllib.error.URLError, urllib.error.HTTPError, TimeoutError, json.JSONDecodeError, OSError) as e:
            if attempt < RETRIES:
                wait = 2 ** attempt
                print(f"[WARN] Wikipedia attempt {attempt}/{RETRIES} failed: {e} — retrying in {wait}s", file=sys.stderr)
                time.sleep(wait)
            else:
                print(f"[ERROR] Wikipedia failed after {RETRIES} retries: {e}", file=sys.stderr)
                return []
    pages = data.get("query", {}).get("pages", {}).values()
    extracts = []
    for page in pages:
        text = page.get("extract", "").strip()
        if text:
            extracts.append(text)
    return extracts


def split_sentences(text):
    raw = re.split(r"\.\s+", text)
    sentences = []
    for s in raw:
        s = s.strip()
        if len(s) >= MIN_SENTENCE_LEN:
            sentences.append(s)
    return sentences


def main():
    count = 0
    seen = set()

    for i in range(ITERATIONS):
        if count >= MAX_SENTENCES:
            break

        extracts = fetch_random_extracts(BATCH_SIZE)
        for text in extracts:
            for sentence in split_sentences(text):
                lower = sentence.lower()
                if lower in seen:
                    continue
                seen.add(lower)

                record = {
                    "text": sentence,
                    "source": "Wikipedia",
                    "title": "Wikipedia",
                    "author": "Wikipedia contributors",
                }
                sys.stdout.write(json.dumps(record) + "\n")
                count += 1
                if count >= MAX_SENTENCES:
                    break

        if (i + 1) % 10 == 0:
            print(f"[INFO] Wikipedia: {count} sentences so far...", file=sys.stderr)

        time.sleep(DELAY)

    print(f"[INFO] Wikipedia: fetched {count} sentences", file=sys.stderr)


if __name__ == "__main__":
    main()
