"""Fetch quotes from DummyJSON Quotes API (free, no auth)."""

import json
import sys
import time
import urllib.request
import urllib.error


API_URL = "https://dummyjson.com/quotes"
MAX_QUOTES = 5000
PAGE_SIZE = 30
DELAY = 0.1
RETRIES = 3
BASE_TIMEOUT = 30
USER_AGENT = "ctypr-content-builder/1.0"


def fetch_url(url, retries=RETRIES, timeout=BASE_TIMEOUT):
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    for attempt in range(1, retries + 1):
        try:
            with urllib.request.urlopen(req, timeout=timeout) as resp:
                return json.loads(resp.read())
        except (urllib.error.URLError, urllib.error.HTTPError, TimeoutError, json.JSONDecodeError, OSError) as e:
            if attempt < retries:
                wait = 2 ** attempt
                print(f"[WARN] DummyJSON attempt {attempt}/{retries} failed: {e} — retrying in {wait}s", file=sys.stderr)
                time.sleep(wait)
            else:
                print(f"[ERROR] DummyJSON failed after {retries} retries: {e}", file=sys.stderr)
                return None


def main():
    count = 0
    skip = 0

    while count < MAX_QUOTES:
        url = f"{API_URL}?limit={PAGE_SIZE}&skip={skip}"
        data = fetch_url(url)
        if data is None:
            break

        quotes = data.get("quotes", [])
        if not quotes:
            break

        for q in quotes:
            record = {
                "text": q.get("quote", "").strip(),
                "source": "DummyJSON",
                "title": "DummyJSON",
                "author": q.get("author", "Unknown"),
            }
            if record["text"]:
                sys.stdout.write(json.dumps(record) + "\n")
                count += 1

        total = data.get("total", 0)
        if skip + PAGE_SIZE >= total:
            break

        skip += PAGE_SIZE
        time.sleep(DELAY)

    print(f"[INFO] DummyJSON: fetched {count} quotes", file=sys.stderr)


if __name__ == "__main__":
    main()
