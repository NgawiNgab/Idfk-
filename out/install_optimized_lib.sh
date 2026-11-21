#!/usr/bin/env bash
# install_optimized_lib.sh -- Deploy the optimized Bedrock game library.
#
# WHAT THIS DOES
#   Replaces the app's native game library with a size-optimized copy.
#   The original 342,231,392-byte libminecraftpe.so is stored uncompressed
#   in the vendor ZIP (Stored, 0% - so the APK carries the full 342 MB).
#   Here it is xz-compressed to ~59 MB (5.8x) and integrity-checked.
#
# WHAT THIS DOES *NOT* DO
#   It does not alter executable code. Bedrock 1.26.66.1 ships with Google
#   Play Integrity (libpairipcore.so); patching .text breaks the signature
#   and the app refuses to start. Any real "engine optimization" at the
#   binary level must be done by the app vendor.
#
# USAGE
#   ./install_optimized_lib.sh <target-dir-with-libminecraftpe.so>
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TARGET="${1:?usage: $0 <dir containing libminecraftpe.so>}"
XZ="$HERE/libminecraftpe_optimized.so.xz"

command -v xz >/dev/null || { echo "!! xz not installed"; exit 1; }
[ -f "$XZ" ] || { echo "!! missing $XZ"; exit 1; }

echo "verifying archive..."
xz -t "$XZ"
echo "decompressing -> $TARGET/libminecraftpe.so"
xz -dc "$XZ" > "$TARGET/libminecraftpe.so.new"
mv "$TARGET/libminecraftpe.so.new" "$TARGET/libminecraftpe.so"
echo "done."
md5sum "$TARGET/libminecraftpe.so"
echo "expected md5: fa377b0e07e7558b16d3684984598cc2"
