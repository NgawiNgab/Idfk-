"""Validate and package the Android module without adding any launcher/game binaries."""

import argparse
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import tempfile
from zipfile import ZIP_DEFLATED, ZipFile

ENTRY = "libore_outline.so"
MANIFEST = Path(__file__).resolve().parents[1] / "package" / "manifest.json"


def readelf(library: Path, option: str) -> str:
    return subprocess.run(
        ["readelf", "--wide", option, str(library.resolve())],
        check=True,
        text=True,
        stdout=subprocess.PIPE,
        env={**os.environ, "LC_ALL": "C"},
    ).stdout


def validate_library(library: Path) -> None:
    with library.open("rb") as f:
        header = f.read(64)
    if (library.name != ENTRY or len(header) != 64 or header[:7] != b"\x7fELF\x02\x01\x01"
            or struct.unpack_from("<HH", header, 16) != (3, 183)):
        raise ValueError(f"Expected an ELF64 little-endian AArch64 shared object named {ENTRY}")
    if ".note.android.ident" not in readelf(library, "--notes"):
        raise ValueError("Missing Android NDK identification note")

    dynamic = readelf(library, "--dynamic")
    needed = set(re.findall(r"\(NEEDED\).*\[([^\]]+)\]", dynamic))
    required = {"libpreloader.so", "libc++_shared.so"}
    allowed = required | {"libandroid.so", "libc.so", "libdl.so", "liblog.so", "libm.so"}
    if not required <= needed <= allowed:
        raise ValueError(f"Unexpected module dependencies: {', '.join(sorted(needed))}")
    if re.search(r"\((?:RUNPATH|RPATH)\)", dynamic):
        raise ValueError("The module may not embed build-machine library search paths")

    symbols = (line.split() for line in readelf(library, "--dyn-syms").splitlines())
    if not any(len(s) >= 8 and s[3:6] == ["FUNC", "GLOBAL", "DEFAULT"]
               and s[6] != "UND" and s[7] == "PLGetModRegistration" for s in symbols):
        raise ValueError("Missing exported PLGetModRegistration")
    loads = [line.split() for line in readelf(library, "--program-headers").splitlines()
             if line.lstrip().startswith("LOAD ")]
    if not loads or any(int(row[-1], 0) < 16384 for row in loads):
        raise ValueError("All LOAD segments must support 16 KiB page alignment")


def write_package(library: Path, output: Path) -> None:
    if output.suffix != ".levipack":
        raise ValueError("Output must end in .levipack")
    manifest = MANIFEST.read_bytes()
    spec = json.loads(manifest)
    if spec.get("type") != "preload-native" or spec.get("entry") != ENTRY:
        raise ValueError("Manifest must describe the root preload-native entry")
    validate_library(library)

    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=output.parent) as temp:
        archive_path = Path(temp) / output.name
        with ZipFile(archive_path, "w", compression=ZIP_DEFLATED) as archive:
            archive.writestr("manifest.json", manifest)
            archive.write(library, ENTRY)
        with ZipFile(archive_path) as archive:
            if archive.namelist() != ["manifest.json", ENTRY] or archive.testzip() is not None:
                raise ValueError("Invalid .levipack layout or CRC")
        archive_path.replace(output)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--library", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    try:
        write_package(args.library, args.output)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"Package error: {error}\n")
    print(args.output)


if __name__ == "__main__":
    main()
