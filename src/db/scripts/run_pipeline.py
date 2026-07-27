"""Orchestrate the full content DB pipeline: fetch → clean → analyze → score → build_db."""

import argparse
import json
import os
import subprocess
import sys
import tempfile
import time


SCRIPTS = os.path.dirname(os.path.abspath(__file__))
PIPE_DIR = os.path.join(SCRIPTS, "build_cache")
SENTENCES_DIR = os.path.join(PIPE_DIR, "fetch_sentences")


DEFAULT_TIMEOUT = 300


def log(msg):
    print(f"[pipeline] {msg}", file=sys.stderr)


def run(cmd, desc, timeout=DEFAULT_TIMEOUT):
    log(f"{desc}...")
    t0 = time.time()
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    try:
        out, err = p.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        p.kill()
        out, err = p.communicate()
        log(f"TIMEOUT after {time.time() - t0:.0f}s: {desc}")
        return None, err.decode("utf-8", errors="replace"), -1
    elapsed = time.time() - t0
    err_text = err.decode("utf-8", errors="replace")
    for line in err_text.strip().splitlines():
        log(f"  {line.strip()}")
    if p.returncode != 0:
        log(f"FAILED ({p.returncode}) in {elapsed:.0f}s: {desc}")
        return None, err_text, p.returncode
    log(f"OK ({elapsed:.0f}s)")
    return out.decode("utf-8", errors="replace"), err_text, p.returncode


def fetch_source(script, args, label):
    """Run a single fetch script, return JSONL text or None on failure."""
    cmd = [sys.executable, os.path.join(SCRIPTS, "fetch", script)] + args
    out, _, rc = run(cmd, f"fetch {label}")
    if rc != 0 or not out:
        log(f"  -> skipped (no results)")
        return None
    return out


def merge_outputs(*jsonl_parts):
    """Merge JSONL from multiple sources, filtering out None."""
    parts = [p for p in jsonl_parts if p]
    if not parts:
        return ""
    return "\n".join(p.rstrip("\n") for p in parts) + "\n"


def write_jsonl(path, text):
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write(text)


def run_pipeline(args):
    os.makedirs(PIPE_DIR, exist_ok=True)

    # ── Step 1: Fetch sentences ──
    sentences_jsonl = None
    if not args.words_only:
        cached_files = []
        if os.path.isdir(SENTENCES_DIR):
            cached_files = sorted(
                os.path.join(SENTENCES_DIR, f)
                for f in os.listdir(SENTENCES_DIR)
                if f.endswith(".jsonl")
            )

        parts = []
        fresh_fetch = False

        log("")
        log("=" * 50)
        log("FETCHING SENTENCES")
        log("=" * 50)

        # Network fetchers (skipped with --no-fetch)
        if not args.no_fetch:
            for script, label in [("dummyjson.py", "dummyjson"), ("wikipedia.py", "wikipedia"), ("news.py", "news")]:
                out = fetch_source(script, [], label)
                parts.append(out)
                if out:
                    fresh_fetch = True
        else:
            log("  --no-fetch: skipping network sources")

        # Local files (always processed when --files is given)
        if args.files:
            files_args = []
            for p in args.files:
                files_args += ["--path", p]
            if args.file_glob:
                files_args += ["--glob", args.file_glob]
            out = fetch_source("files.py", files_args, "files")
            parts.append(out)
            if out:
                fresh_fetch = True

        # Cache the fresh results
        sentences_jsonl = merge_outputs(*parts)
        if fresh_fetch and sentences_jsonl:
            ts = time.strftime("%Y%m%d_%H%M%S")
            os.makedirs(SENTENCES_DIR, exist_ok=True)
            write_jsonl(
                os.path.join(SENTENCES_DIR, f"sentences_{ts}.jsonl"),
                sentences_jsonl,
            )

        # Fall back to cached files if fresh fetch had no results
        if not sentences_jsonl and cached_files:
            log(f"Using {len(cached_files)} cached fetch file(s)")
            parts = []
            for cf in cached_files:
                with open(cf, "r", encoding="utf-8") as f:
                    parts.append(f.read())
            sentences_jsonl = merge_outputs(*parts)

        if not sentences_jsonl:
            log("WARNING: no sentence data available")

    # ── Step 2: Fetch words ──
    words_jsonl = None
    if not args.sentences_only:
        need_fetch = not args.no_fetch
        words_cache = os.path.join(PIPE_DIR, "words.jsonl")

        if need_fetch or not os.path.exists(words_cache):
            log("")
            log("=" * 50)
            log("FETCHING WORDS")
            log("=" * 50)
            out, _, rc = run(
                [sys.executable, os.path.join(SCRIPTS, "fetch", "words.py")],
                "fetch words",
                timeout=300,
            )
            if rc == 0 and out:
                words_jsonl = out
                write_jsonl(words_cache, out)
        else:
            log("Using cached words.jsonl")
            with open(words_cache, "r", encoding="utf-8") as f:
                words_jsonl = f.read()

        if not words_jsonl:
            log("WARNING: no word data available")

    # ── Step 3: Clean + Analyze + Score (sentences) ──
    scored_sentences = None
    if sentences_jsonl:
        log("")
        log("=" * 50)
        log("SENTENCE PIPELINE: clean → analyze → score")
        log("=" * 50)

        # Clean
        out, _, rc = run(
            [sys.executable, os.path.join(SCRIPTS, "clean.py")],
            "clean",
            input_data=sentences_jsonl,
        )
        if rc != 0 or not out:
            log("Clean step produced no output, skipping sentence pipeline")
        else:
            cleaned = out

            # Analyze
            out, _, rc = run(
                [sys.executable, os.path.join(SCRIPTS, "analyze.py")],
                "analyze",
                input_data=cleaned,
            )
            if rc != 0 or not out:
                log("Analyze step produced no output, skipping score")
            else:
                analyzed = out

                # Score
                out, _, rc = run(
                    [sys.executable, os.path.join(SCRIPTS, "score.py")],
                    "score",
                    input_data=analyzed,
                )
                if rc == 0 and out:
                    scored_sentences = out

    # ── Step 4: Score words (pass-through) ──
    scored_words = None
    if words_jsonl:
        out, _, rc = run(
            [sys.executable, os.path.join(SCRIPTS, "score.py")],
            "score words",
            input_data=words_jsonl,
        )
        if rc == 0 and out:
            scored_words = out

    # ── Step 5: Build databases ──
    log("")
    log("=" * 50)
    log("BUILD DATABASES")
    log("=" * 50)

    build_cmd = [sys.executable, os.path.join(SCRIPTS, "build_db.py")]

    if scored_sentences and not args.words_only:
        sf = tempfile.NamedTemporaryFile(
            mode="w", suffix=".jsonl", delete=False, encoding="utf-8"
        )
        sf.write(scored_sentences)
        sf.close()
        build_cmd += ["--sentences", sf.name, "--sentences-db", args.sentences_db]
    else:
        sf = None

    if scored_words and not args.sentences_only:
        wf = tempfile.NamedTemporaryFile(
            mode="w", suffix=".jsonl", delete=False, encoding="utf-8"
        )
        wf.write(scored_words)
        wf.close()
        build_cmd += ["--words", wf.name, "--words-db", args.words_db]
    else:
        wf = None

    if len(build_cmd) > 2:
        _, _, rc = run(build_cmd, "build_db")
    else:
        log("Nothing to build")

    # Cleanup temp files
    for f in [sf, wf]:
        if f:
            os.unlink(f.name)


def run_with_input(cmd, desc, timeout=DEFAULT_TIMEOUT, input_data=None):
    """Like run() but accepts stdin data."""
    log(f"{desc}...")
    t0 = time.time()
    p = subprocess.Popen(
        cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    stdin_bytes = input_data.encode("utf-8") if input_data else None
    try:
        out, err = p.communicate(input=stdin_bytes, timeout=timeout)
    except subprocess.TimeoutExpired:
        p.kill()
        out, err = p.communicate()
        log(f"TIMEOUT after {time.time() - t0:.0f}s: {desc}")
        return None, err.decode("utf-8", errors="replace"), -1
    elapsed = time.time() - t0
    err_text = err.decode("utf-8", errors="replace")
    for line in err_text.strip().splitlines():
        log(f"  {line.strip()}")
    if p.returncode != 0:
        log(f"FAILED ({p.returncode}) in {elapsed:.0f}s: {desc}")
        return None, err_text, p.returncode
    log(f"OK ({elapsed:.0f}s)")
    return out.decode("utf-8", errors="replace"), err_text, p.returncode


# Patch run() to use run_with_input when input_data is provided
_orig_run = run


def run(cmd, desc, timeout=DEFAULT_TIMEOUT, input_data=None):
    if input_data is not None:
        return run_with_input(cmd, desc, timeout, input_data)
    return _orig_run(cmd, desc, timeout)


def main():
    parser = argparse.ArgumentParser(
        description="Build ctypr content databases from scratch",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Examples:\n"
            "  python run_pipeline.py\n"
            "  python run_pipeline.py --sentences-db ../sentences.db --words-db ../words.db\n"
            "  python run_pipeline.py --files ./my_texts/ --no-fetch\n"
            "  python run_pipeline.py --words-only --words-db ../words.db\n"
        ),
    )

    parser.add_argument(
        "--sentences-db",
        default="sentences.db",
        help="Output path for sentences database (default: sentences.db)",
    )
    parser.add_argument(
        "--words-db",
        default="words.db",
        help="Output path for words database (default: words.db)",
    )

    parser.add_argument(
        "--files",
        action="append",
        default=[],
        help="Local file/directory to include (repeatable, passed to fetch/files.py)",
    )
    parser.add_argument(
        "--file-glob",
        default="*.txt",
        help="Glob pattern for local files (default: *.txt)",
    )

    parser.add_argument(
        "--no-fetch",
        action="store_true",
        help="Skip network fetchers (dummyjson, wikipedia, news); local files via --files still work",
    )
    parser.add_argument(
        "--sentences-only",
        action="store_true",
        help="Only build sentences database",
    )
    parser.add_argument(
        "--words-only",
        action="store_true",
        help="Only build words database",
    )

    args = parser.parse_args()

    if args.sentences_only and args.words_only:
        parser.error("--sentences-only and --words-only are mutually exclusive")

    log(f"Pipeline starting (intermediate files: {PIPE_DIR})")
    log(f"Output: sentences={args.sentences_db}, words={args.words_db}")
    t0 = time.time()

    run_pipeline(args)

    elapsed = time.time() - t0
    log(f"")
    log(f"{'=' * 50}")
    log(f"PIPELINE COMPLETE ({elapsed:.0f}s)")
    log(f"  sentences db: {args.sentences_db}")
    log(f"  words db:     {args.words_db}")
    log(f"{'=' * 50}")


if __name__ == "__main__":
    main()
