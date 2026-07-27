"""Fetch quotes from the Quotable API."""

import json
import sys
import time
import urllib.request
import urllib.error


API_URL = "https://api.quotable.io/quotes"
MAX_QUOTES = 5000
PAGE_SIZE = 150
DELAY = 0.1


def fetch_page(page):
    url = f"{API_URL}?limit={PAGE_SIZE}&maxLength=250&page={page}"
    try:
        with urllib.request.urlopen(url, timeout=15) as resp:
            data = json.loads(resp.read())
    except (urllib.error.URLError, json.JSONDecodeError) as e:
        print(f"[ERROR] Quotable page {page}: {e}", file=sys.stderr)
        return None
    return data


def main():
    count = 0
    page = 1

    while count < MAX_QUOTES:
        data = fetch_page(page)
        if data is None:
            break

        quotes = data.get("results", [])
        if not quotes:
            break

        for q in quotes:
            record = {
                "text": q.get("content", "").strip(),
                "source": "Quotable",
                "title": "Quotable",
                "author": q.get("author", "Unknown"),
            }
            if record["text"]:
                sys.stdout.write(json.dumps(record) + "\n")
                count += 1

        total = data.get("totalCount", 0)
        if page * PAGE_SIZE >= total:
            break

        page += 1
        time.sleep(DELAY)

    print(f"[INFO] Quotable: fetched {count} quotes", file=sys.stderr)


if __name__ == "__main__":
    main()
