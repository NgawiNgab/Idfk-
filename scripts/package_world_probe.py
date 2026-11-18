"""Package the stand-alone Bedrock behavior pack. No native libraries are included."""

import argparse
import json
from pathlib import Path
from tempfile import TemporaryDirectory
from zipfile import ZIP_DEFLATED, ZipFile

ROOT = Path(__file__).resolve().parents[1] / "probes" / "one-block"
FILES = ("manifest.json", "scripts/main.js", "scripts/probe.js")


def package(output: Path) -> None:
    if output.suffix != ".mcpack":
        raise ValueError("Behavior pack output must end in .mcpack")
    manifest = json.loads((ROOT / "manifest.json").read_text(encoding="utf-8"))
    entries = [m["entry"] for m in manifest["modules"] if m["type"] == "script"]
    if entries != ["scripts/main.js"]:
        raise ValueError("Unexpected script entry in the behavior pack")

    output.parent.mkdir(parents=True, exist_ok=True)
    with TemporaryDirectory(dir=output.parent) as temp:
        path = Path(temp) / output.name
        with ZipFile(path, "w", compression=ZIP_DEFLATED) as archive:
            for name in FILES:
                archive.write(ROOT / name, name)
        with ZipFile(path) as archive:
            if tuple(archive.namelist()) != FILES or archive.testzip() is not None:
                raise ValueError("Invalid behavior pack archive")
        path.replace(output)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    try:
        package(args.output)
    except (OSError, ValueError) as error:
        parser.exit(1, f"Pack error: {error}\n")
    print(args.output)
