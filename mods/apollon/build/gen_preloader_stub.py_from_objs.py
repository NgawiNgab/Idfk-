#!/usr/bin/env python3
"""
gen_preloader_stub.py_from_objs.py -- emit libpreloader.so with only the pl::*
symbols our objects actually reference.

WHY THIS EXISTS
---------------
LeviLaunchpad dlopen()s the mod with BIND_NOW, and it loads libpreloader.so
*after* doing so. Every undefined symbol must therefore resolve at load time,
which means the mod must declare libpreloader.so in DT_NEEDED. To get that entry
the linker has to be handed a library with that SONAME.

The Ore ESP reference mod solves this with gen_preloader_stub.py, but that
script requires an Android NDK. This build is zig-based, so we do the same job
here without a NDK, and we read the symbols from the OBJECT FILES rather than
from a finished .so (we need the stub before we can link).

The stub is a link-time artifact only. It is NOT packaged; the launcher's real
libpreloader.so provides these symbols at runtime.

Usage:
    gen_preloader_stub.py_from_objs.py <obj-dir> <output-dir>

Exit codes:
    0  stub written (or already up to date)
    0  nothing to do -- no pl::* references (stub removed so no phantom NEEDED)
"""
from __future__ import annotations

import os
import re
import subprocess
import sys
from pathlib import Path

# Everything the preloader exports that a mod may call:
#   pl::mod::*       lifecycle / NativeMod / ModContext
#   pl::modmenu::*   the Mod Menu registry
#   pl::log::*       logging
FUNC_PREFIXES = ("_ZN2pl", "_ZNK2pl", "_ZN3pl", "_ZNK3pl")
DATA_PREFIXES = ("_ZTIN2pl", "_ZTSN2pl", "_ZTVN2pl", "_ZTIN3pl", "_ZTSN3pl", "_ZTVN3pl")
PLAIN_PREFIXES = ("PLGetModRegistration",)

# C / C++ ABI imports that come from libc++_shared or bionic, not the launcher.
# Guarding against these avoids us "providing" std:: symbols in the stub.
def is_preloader_symbol(name: str) -> bool:
    if name.startswith(PLAIN_PREFIXES):
        return True
    return name.startswith(FUNC_PREFIXES + DATA_PREFIXES)


def undefined_from_objects(objdir: Path) -> list[str]:
    """Union of undefined symbols across every .o in `objdir`."""
    syms: set[str] = set()
    for obj in sorted(objdir.glob("*.o")):
        r = subprocess.run(["nm", "-u", str(obj)], capture_output=True, text=True)
        for line in r.stdout.splitlines():
            parts = line.split()
            if len(parts) >= 2 and parts[0] == "U":
                syms.add(parts[1].split("@")[0])
    return sorted(syms)


def main() -> int:
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    objdir = Path(sys.argv[1])
    outdir = Path(sys.argv[2])
    outdir.mkdir(parents=True, exist_ok=True)

    stub = outdir / "libpreloader.so"
    src = outdir / "preloader_stub.cpp"

    all_undef = undefined_from_objects(objdir)
    pl = [s for s in all_undef if is_preloader_symbol(s)]

    if not pl:
        # No launcher API referenced. Remove any stale stub so we never emit a
        # DT_NEEDED entry for a library we do not actually use.
        if stub.exists():
            stub.unlink()
            print("no pl::* references -- removed stale libpreloader.so stub")
        else:
            print("no pl::* references -- no stub needed")
        return 0

    print(f"launcher symbols referenced: {len(pl)}")
    for s in pl:
        print(f"  {s}")

    lines = [
        "// AUTO-GENERATED -- link-time import stub, never shipped.",
        "// Real definitions live in LeviLaunchpad's libpreloader.so.",
        'extern "C" {',
    ]
    for s in pl:
        safe = re.sub(r"\W", "_", s)
        # The asm label pins the exact mangled name, so we never have to
        # reproduce C++ signature mangling by hand.
        lines.append(f'void {safe}(void) asm("{s}");')
        lines.append(f"void {safe}(void) {{ }}")
    lines.append("}")
    text = "\n".join(lines) + "\n"

    # Only recompile when the symbol set changed; keeps iterative builds fast.
    if src.exists() and src.read_text() == text and stub.exists():
        print("stub up to date")
        return 0
    src.write_text(text)

    zig = os.environ.get("ZIG", "/tmp/venv/bin/python -m ziglang")
    libcxx = os.environ.get("LIBCXX_INCLUDE", "/tmp/libcxx-android")
    sysroot = os.environ.get("ANDROID_SYSROOT", "/tmp/sysroot")
    cmd = (
        f"{zig} c++ -target aarch64-linux-android -shared -fPIC -O0 "
        f"-nostdlib -nostdinc "
        f"-I {libcxx} "
        f"-I {sysroot}/usr/include/aarch64-linux-android "
        f"-I {sysroot}/usr/include "
        f"-Wl,-soname,libpreloader.so "
        f"-o {stub} {src}"
    )
    r = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    if r.returncode != 0:
        print("!! stub build failed")
        print(r.stdout[-1500:])
        print(r.stderr[-1500:])
        return r.returncode

    dyn = subprocess.run(["readelf", "-d", str(stub)], capture_output=True, text=True).stdout
    soname = [l.split("[")[-1].rstrip("]") for l in dyn.splitlines() if "SONAME" in l]
    print("stub SONAME:", soname[0] if soname else "MISSING")
    if not soname or soname[0] != "libpreloader.so":
        print("!! SONAME is wrong; DT_NEEDED would be incorrect")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
