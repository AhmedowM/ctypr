# scripts/setup-hooks.ps1
# Installs the commit-msg hook from .githooks/ into the local .git/hooks directory.
# Run once after cloning:
#   powershell -ExecutionPolicy Bypass -File scripts/setup-hooks.ps1

$RepoRoot = Split-Path -Parent (Split-Path -Parent $PSCommandPath)
$HookSource = Join-Path $RepoRoot ".githooks" "commit-msg"
$HookDest = Join-Path $RepoRoot ".git" "hooks" "commit-msg"

if (-not (Test-Path $HookSource)) {
    Write-Error "No hook found at $HookSource"
    exit 1
}

Copy-Item -LiteralPath $HookSource -Destination $HookDest -Force
Write-Host "Installed commit-msg hook -> $HookDest"
