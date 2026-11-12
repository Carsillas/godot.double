import importlib.util
import tempfile
import unittest
from pathlib import Path


spec = importlib.util.spec_from_file_location(
    "shared_texture_builders", Path(__file__).resolve().parents[1] / "shared_texture_builders.py"
)
builders = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builders)


class TextureBuilderTests(unittest.TestCase):
    def test_discovery_and_embedding(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "ui").mkdir()
            (root / "ui" / "panel.png").write_bytes(b"\x00\x7f\xff")
            (root / "a.svg").write_bytes(b"<svg/>")
            (root / "ignored.png.import").write_text("not an image")
            sources = builders.find_textures(root)
            self.assertEqual(
                [Path(p).relative_to(root).as_posix() for p in sources], ["a.svg", "ui/panel.png"]
            )
            output = root / "textures.gen.h"
            builders.generate_header(output, reversed(sources), root)
            generated = output.read_text()
            self.assertIn('embedded_texture_1, 3', generated)
            self.assertIn('"ui/panel.png"', generated)
            self.assertRegex(generated, r"0,\s+127,\s+255")
            self.assertIn("embedded_texture_count = 2", generated)
            builders.generate_header(output, sources, root)
            self.assertEqual(output.read_text(), generated)
            # Removed files disappear from the generated catalog on the next scan.
            (root / "a.svg").unlink()
            builders.generate_header(output, builders.find_textures(root), root)
            self.assertNotIn('"a.svg"', output.read_text())

    def test_manifest_path_ids(self):
        from modules.shared_textures.texture_manifest import sync_manifest
        import json

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            image = root / "original.png"
            image.write_bytes(b"test-image")
            self.assertEqual(sync_manifest(root)["original.png"], "original.png")
            image.write_bytes(b"replacement-image")
            self.assertEqual(sync_manifest(root)["original.png"], "original.png")
            (root / "manifest.gen.json").unlink()
            self.assertEqual(sync_manifest(root)["original.png"], "original.png")
            manifest = json.loads((root / "manifest.gen.json").read_text())
            self.assertEqual(manifest, {"assets": [{"id": "original.png", "path": "original.png"}]})
            image.unlink()
            self.assertEqual(sync_manifest(root), {})
            self.assertEqual(json.loads((root / "manifest.gen.json").read_text()), {"assets": []})

    def test_empty_catalog(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "textures.gen.h"
            builders.generate_header(output, [], directory)
            self.assertIn("{ nullptr, nullptr, nullptr, 0 }", output.read_text())
            self.assertIn("embedded_texture_count = 0", output.read_text())

    def test_unsafe_names_and_empty_images(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            output = root / "textures.gen.h"
            (root / "empty.png").touch()
            for name in ["comma,name.png", "colon:name.svg", "UPPER.PNG", "empty.png"]:
                with self.subTest(name=name):
                    # Test names before reading files so this is portable to Windows.
                    with self.assertRaises(ValueError):
                        builders.generate_header(output, [root / name], root)


if __name__ == "__main__":
    unittest.main()
