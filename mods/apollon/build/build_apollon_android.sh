#!/usr/bin/env bash
# build_apollon_android.sh -- Reproducible aarch64 Android build of Apollon ImGui v4.62
#
# Why this exists: Apollon ships only Android.mk for a 2019-era NDK. This script
# cross-compiles the same sources for arm64-v8a Android using Zig's clang with a
# real bionic r28 sysroot, linking against libc++_shared (the ABI Minecraft
# itself uses), NEON/statically-linked Dobby inline hooks, and 16 KB pages.
#
# Inputs (already in this repo):
#   mods/apollon/source/Apollon_Client_ImGui/app/src/main/jni   original sources
#   mods/apollon/game/libminecraftpe.so                         target game library
#
# External sysroot (one-time):
#   git clone --depth 1 https://github.com/gubaojian/sysroot /tmp/sysroot
#   -> bionic headers incl. jni.h, GLES3, EGL, android/*, sys/system_properties.h
#
# Usage:  ./build_apollon_android.sh [clean]
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
AP="$(cd "$HERE/.." && pwd)"
SRC="$AP/source/Apollon_Client_ImGui/app/src/main/jni"
WS="$AP/build"
OBJ="$WS/obj_android"
OUT="$WS/out"
SYSROOT="${ANDROID_SYSROOT:-/tmp/sysroot}"
ZIG="${ZIG:-/tmp/venv/bin/python -m ziglang}"
INC_CXX="${LIBCXX_INCLUDE:-/tmp/libcxx-android}"      # libc++ headers + __config_site
# clang's builtin-include dir ships stddef.h/stdarg.h that libc++'s <stddef.h>
# defers to via #include_next. Without it on the search path <cstddef> trips
# libc++'s own guard. Derived from the zig installation so it never goes stale.
INC_RSRC="${LIBCXX_RESOURCE_INCLUDE:-$(/tmp/venv/bin/python -c 'import ziglang,os;print(os.path.join(os.path.dirname(ziglang.__file__),"lib","include"))' 2>/dev/null)}"
STUBS="$WS/stublibs"                                   # empty .so stubs for DT_NEEDED
DOBBY="$SRC/libraries/arm64-v8a/libdobby.a"
# compiler-rt: we link -nostdlib, so clang's out-of-line atomics
# (__aarch64_cas4_acq_rel, __aarch64_ldadd{4,8}_*, ...) are NOT pulled in
# automatically. Without this archive every std::atomic CAS/RMW produces an
# undefined symbol and dlopen fails. Built once from zig's bundled compiler-rt:
#   zig build-lib -target aarch64-linux-android -O ReleaseFast \\
#       -fcompiler-rt --name crt_a64 -femit-bin=$CRT_A64
CRT_A64="${COMPILER_RT_AARCH64:-/tmp/crt/libcrt_a64.a}"

[ "$(id -u)" != 0 ] || true
[ -d "$SYSROOT/usr/include" ] || { echo "!! missing bionic sysroot at $SYSROOT"; exit 1; }
[ -d "$INC_CXX" ]            || { echo "!! missing libc++ include at $INC_CXX"; exit 1; }
[ -f "$DOBBY" ]              || { echo "!! missing $DOBBY"; exit 1; }
if [ ! -f "$CRT_A64" ]; then
  echo "!! missing compiler-rt archive at $CRT_A64"
  echo "   build it with:"
  echo "   $ZIG build-lib -target aarch64-linux-android -O ReleaseFast -fcompiler-rt \\"
  echo "        --name crt_a64 -femit-bin=$CRT_A64"
  exit 1
fi

if [ "${1:-}" = "clean" ]; then rm -rf "$OBJ" "$OUT"; fi
mkdir -p "$OBJ" "$OUT"

# --- libc++ configuration (zig's libc++ expects -D, it patches out __config_site)
CXX_DEFS=(
  -D_LIBCPP_HARDENING_MODE_DEFAULT=_LIBCPP_HARDENING_MODE_NONE
  -D_LIBCPP_ASSERTION_SEMANTIC_DEFAULT=_LIBCPP_ASSERTION_SEMANTIC_IGNORE
  -D_LIBCPP_ABI_NAMESPACE=__ndk1 -D_LIBCPP_ABI_NAMESPACE_LEN=6 -D_LIBCPP_ABI_VERSION=1
  -D_LIBCPP_HAS_THREADS=1 -D_LIBCPP_HAS_MONOTONIC_CLOCK=1 -D_LIBCPP_HAS_MUSL_LIBC=0
  -D_LIBCPP_HAS_THREAD_API_PTHREAD=1 -D_LIBCPP_HAS_FILESYSTEM=1
  -D_LIBCPP_HAS_RANDOM_DEVICE=1 -D_LIBCPP_HAS_LOCALIZATION=1 -D_LIBCPP_HAS_UNICODE=1
  -D_LIBCPP_HAS_WIDE_CHARACTERS=1 -D_LIBCPP_HAS_TIME_ZONE_DATABASE=0
  -D_LIBCPP_INSTRUMENTED_WITH_ASAN=0 -D_LIBCPP_LIBC_PICOLIBC=0
  -D_LIBCPP_LIBC_NEWLIB=0 -D_LIBCPP_LIBC_LLVM_LIBC=0
  -D_LIBCPP_HAS_VENDOR_AVAILABILITY_ANNOTATIONS=0
  -D_LIBCPP_TYPEINFO_COMPARISON_IMPLEMENTATION=1
)
# Include search order, and why it is what it is:
#   -I (searched first)  libc++          -> its <stddef.h> wins over clang's
#   -I (searched first)  bionic arch     -> <asm/*.h>, <machine/*.h>
#   -I (searched first)  bionic          -> <stdint.h>, <pthread.h>, jni.h
#   -isystem (builtin-include comes first among isystem) -> clang's stddef.h
# libc++'s <stddef.h> does `#include_next <stddef.h>`, which reaches clang's.
# Put libc++ in -isystem instead and the builtin dir shadows it, so #include_next
# finds nothing and libc++ errors out. Hence -I for libc++, and -nostdinc so the
# ordering is entirely ours.
COMMON=(
  -target aarch64-linux-android -O2 -fPIC -nostdinc -nostdlib
  -I "$INC_CXX"
  -I "$SYSROOT/usr/include/aarch64-linux-android"
  -I "$SYSROOT/usr/include"
  "${CXX_DEFS[@]}"
  -DANDROID
)
[ -z "$INC_RSRC" ] || COMMON+=(-isystem "$INC_RSRC")
CXXFLAGS=("${COMMON[@]}" -std=c++17 -fpermissive -w -I "$SRC" -I "$SRC/ImGui" -I "$SRC/ImGui/backends" -I "$SRC/Includes")

# NOTE: object names must be PATH-UNIQUE. Apollon has both a class `KittyUtils`
# (KittyMemory/KittyUtils.h) and a namespace `KittyUtils`
# (KittyScanner/KittyUtils.hpp). Naming both objects "KittyUtils.o" silently
# overwrote the class definitions, leaving validateHexString/fromHex undefined
# at dlopen time ("cannot locate symbol ...KittyUtils..."). Fixed by deriving
# the object name from the full relative source path.
objname() { local n="${1//\//_}"; echo "${n%.*}"; }
compile_cxx() { echo "  CXX $1"; $ZIG c++ "${CXXFLAGS[@]}" -c "$SRC/$1" -o "$OBJ/$(objname "$1").o"; }
compile_c()   { echo "  CC  $1"; $ZIG cc "${COMMON[@]}" -w -I "$SRC" -I "$SRC/Substrate" -c "$SRC/$1" -o "$OBJ/$(objname "$1").o"; }

echo "=== compiling Apollon (aarch64-linux-android, API 28) ==="
for f in \
  ImGui/imgui.cpp ImGui/imgui_draw.cpp ImGui/imgui_widgets.cpp ImGui/imgui_tables.cpp \
  ImGui/backends/imgui_impl_opengl3.cpp ImGui/backends/imgui_impl_android.cpp \
  KittyMemory/KittyMemory.cpp KittyMemory/MemoryPatch.cpp KittyMemory/MemoryBackup.cpp KittyMemory/KittyUtils.cpp \
  KittyScanner/KittyMemory_new.cpp KittyScanner/KittyScanner.cpp KittyScanner/KittyUtils.cpp \
  Substrate/SubstrateDebug.cpp Substrate/SubstrateHook.cpp Substrate/SubstratePosixMemory.cpp \
  Arm64/And64InlineHook.cpp ENCRYPT/oxorany.cpp main.cpp ; do
  compile_cxx "$f"
done
compile_c Substrate/hde64.c

echo "=== linking libapollon_imgu.so ==="
$ZIG c++ -target aarch64-linux-android -shared -fPIC -O2 -nostdlib -nostdinc \
  -o "$OUT/libapollon_imgu.so" "$OBJ"/*.o "$DOBBY" "$CRT_A64" \
  -Wl,--gc-sections -Wl,--strip-all \
  -Wl,-soname,libapollon_imgu.so \
  -L"$STUBS" -Wl,--no-as-needed \
  -lGLESv2 -lEGL -llog -landroid -lz -ldl -lm -lc++_shared \
  -Wl,-z,max-page-size=16384

echo "=== done: $OUT/libapollon_imgu.so ==="
ls -la "$OUT/libapollon_imgu.so"
readelf -d "$OUT/libapollon_imgu.so" | grep -E "NEEDED|SONAME" || true
