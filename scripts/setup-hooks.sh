#!/usr/bin/env bash
# scripts/setup-hooks.sh
# Installs the commit-msg hook from .githooks/ into the local .git/hooks directory.
# Run once after cloning: bash scripts/setup-hooks.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
HOOK_SOURCE="$REPO_ROOT/.githooks/commit-msg"
HOOK_DEST="$REPO_ROOT/.git/hooks/commit-msg"

if [ ! -f "$HOOK_SOURCE" ]; then
    echo "ERROR: No hook found at $HOOK_SOURCE. Did you clone the full repo?" >&2
    exit 1
fi

cp "$HOOK_SOURCE" "$HOOK_DEST"
chmod +x "$HOOK_DEST"
echo "Installed commit-msg hook -> $HOOK_DEST"
