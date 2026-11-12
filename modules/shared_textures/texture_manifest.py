"""Generate a disposable catalog of path-based IDs for embedded assets."""

import json
from pathlib import Path

EXTENSIONS = {".png", ".svg", ".jpg", ".jpeg", ".webp"}


def sync_manifest(root, output=None):
    root = Path(root)
    path = Path(output) if output is not None else root / "manifest.gen.json"
    images = sorted(p for p in root.rglob("*") if p.is_file() and p.suffix.lower() in EXTENSIONS)
    entries = []
    for image in images:
        name = image.relative_to(root).as_posix()
        entries.append({"id": name, "path": name})
    data = {"assets": entries}
    serialized = json.dumps(data, indent=2) + "\n"
    if not path.exists() or path.read_text() != serialized:
        path.write_text(serialized, encoding="utf-8", newline="\n")
    return {entry["path"]: entry["id"] for entry in data["assets"]}


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", nargs="?", default=str(Path(__file__).parent / "assets"))
    parser.add_argument("--output", default=str(Path(__file__).parent / "embedded_textures.gen.json"))
    args = parser.parse_args()
    sync_manifest(args.directory, args.output)
