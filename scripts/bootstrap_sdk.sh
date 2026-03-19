#!/usr/bin/env bash
# bootstrap_sdk.sh — Download and extract the Signal Hound SDK.
#
# Environment variables:
#   SIGNALHOUND_SDK_URL      — Download URL (default: official 2026 release)
#   SIGNALHOUND_SDK_ARCHIVE  — Local cache path for the zip (optional)
#   SIGNALHOUND_SDK_SHA256   — SHA256 for integrity check (optional)
#   SIGNALHOUND_SDK_DIR      — Where to extract (default: .sdk_cache)
#
# After running, sets SIGNALHOUND_SDK_ROOT to the extracted directory.

set -euo pipefail

SDK_URL="${SIGNALHOUND_SDK_URL:-https://signalhound.com/sigdownloads/SDK/signal_hound_sdk_02_19_26.zip}"
SDK_DIR="${SIGNALHOUND_SDK_DIR:-.sdk_cache/signal_hound_sdk}"
ARCHIVE="${SIGNALHOUND_SDK_ARCHIVE:-${SIGNALHOUND_SDK_DIR:-.sdk_cache}/sdk.zip}"

mkdir -p "$(dirname "$ARCHIVE")"

if [ ! -f "$ARCHIVE" ]; then
    echo "[bootstrap_sdk] Downloading SDK from $SDK_URL"
    curl -fL --retry 3 -o "$ARCHIVE" "$SDK_URL"
else
    echo "[bootstrap_sdk] Using cached SDK archive: $ARCHIVE"
fi

if [ -n "${SIGNALHOUND_SDK_SHA256:-}" ]; then
    echo "[bootstrap_sdk] Verifying SHA256..."
    echo "$SIGNALHOUND_SDK_SHA256  $ARCHIVE" | sha256sum -c -
fi

if [ ! -d "$SDK_DIR" ]; then
    echo "[bootstrap_sdk] Extracting SDK..."
    mkdir -p "$(dirname "$SDK_DIR")"
    unzip -q "$ARCHIVE" -d "$(dirname "$SDK_DIR")"
fi

echo "[bootstrap_sdk] SDK ready at: $SDK_DIR"
echo "export SIGNALHOUND_SDK_ROOT='$SDK_DIR'"
