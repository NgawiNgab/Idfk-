#!/usr/bin/env bash
set -euo pipefail

root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
ndk=${1:-${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}}
build_dir=${2:-"$root/build/android-arm64"}
if [[ -z "$ndk" || ! -f "$ndk/build/cmake/android.toolchain.cmake" ]]; then
    echo 'Pass the Android NDK r28c directory or set ANDROID_NDK_HOME.' >&2
    exit 1
fi
if ! grep -Eq '^Pkg\.Revision[[:space:]]*=[[:space:]]*28\.2\.13676358[[:space:]]*$' "$ndk/source.properties"; then
    echo 'Expected Android NDK r28c (28.2.13676358), matching the launcher reference.' >&2
    exit 1
fi
ndk=$(cd -- "$ndk" && pwd)

cmake -S "$root" -B "$build_dir" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="$ndk/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-28 -DANDROID_STL=c++_shared
cmake --build "$build_dir" --target ore_outline --parallel "${BUILD_JOBS:-2}"
python3 "$root/scripts/package_levipack.py" \
    --library "$build_dir/mod/libore_outline.so" \
    --output "$build_dir/ore-outline-hud-test.levipack"
