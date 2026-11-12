"""Embed game UI images without requiring a project's resource importer."""

import json
import re
from pathlib import Path

import methods
from modules.shared_textures.texture_manifest import sync_manifest


EXTENSIONS = {".png", ".svg", ".jpg", ".jpeg", ".webp"}


def find_textures(directory):
    return sorted(
        str(path) for path in Path(directory).rglob("*") if path.suffix.lower() in EXTENSIONS and path.is_file()
    )


def make_header(target, source, env):
    generate_header(str(target[0]), source, Path(__file__).resolve().parent / "assets", str(target[1]))


def generate_header(target, source, root, manifest_path=None):
    entries = []
    root = Path(root).resolve()
    identifiers = sync_manifest(root, manifest_path or Path(target).with_suffix(".json"))
    for path in sorted(Path(str(item)) for item in source):
        name = path.resolve().relative_to(root).as_posix()
        if not re.fullmatch(r"[A-Za-z0-9_/-]+\.(png|svg|jpg|jpeg|webp)", name):
            raise ValueError(
                f"Unsupported shared texture name: {name}. "
                "Use letters, digits, underscores, hyphens and folders with lowercase extensions."
            )
        entries.append((name, path.read_bytes()))

    with methods.generated_wrapper(str(target)) as file:
        file.write(
            "struct EmbeddedTextureEntry {\n\tconst char *name;\n\tconst char *id;\n\tconst unsigned char *data;\n\tint size;\n};\n\n"
        )
        for index, (_, data) in enumerate(entries):
            if not data:
                raise ValueError("Shared textures must not be empty.")
            file.write(
                f"inline constexpr unsigned char embedded_texture_{index}[] = {{\n"
                f"{methods.format_buffer(data, 1)}\n}};\n\n"
            )
        file.write("inline constexpr EmbeddedTextureEntry embedded_textures[] = {\n")
        for index, (name, data) in enumerate(entries):
            file.write(f'\t{{ {json.dumps(name)}, {json.dumps(identifiers[name])}, embedded_texture_{index}, {len(data)} }},\n')
        # A sentinel keeps the array valid even if the asset folder is empty.
        file.write("\t{ nullptr, nullptr, nullptr, 0 },\n};\n")
        file.write(f"inline constexpr int embedded_texture_count = {len(entries)};\n")
