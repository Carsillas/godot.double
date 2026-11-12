# Shared textures

`SharedTexture` is a native `Texture2D` resource for game assets embedded in this
engine fork. It works with `TextureRect`, `Sprite2D`, `StyleBoxTexture`, materials,
and other properties accepting a `Texture2D`. It does not change the editor theme.

## Try the preview project

With a compiled editor from this branch, open
`modules/shared_textures/examples/demo/project.godot`. The sample scene shows
scaled and tiled copies of an embedded texture. Select either preview node and
expand **Texture** in the Inspector to try the **Asset** dropdown, or press F6
to run the scene. The project contains no source images and requires no C#.

For a Windows double-precision editor built in this checkout, run from the
repository root:

```powershell
.\bin\godot.windows.editor.double.x86_64.exe --editor --path modules/shared_textures/examples/demo
```

## Add an image

1. Copy the image into `modules/shared_textures/assets/`. Subfolders are supported.
2. Use letters, digits, underscores, and hyphens in names. Extensions must be
   lowercase: `.png`, `.svg`, `.jpg`, `.jpeg`, or `.webp`.
3. Rebuild the editor and export templates with the same asset folder. The
   Possessions GitHub Actions workflow does this automatically for its selected
   branch. Regenerate C# bindings when first adding this module, as that workflow
   already does; changing only images does not change the C# API.

No manual manifest entry or C++ edit is required when adding an image. Images are embedded as their original bytes
and decoded on first selection. SVGs use their intrinsic size at scale 1.0; use
self-contained SVGs with explicit dimensions. The build must enable the relevant
image decoder (the normal engine configuration includes all supported formats).

The menu displays each image's relative path, such as `ui/buttons/confirm.png`.
The manifest uses these relative paths directly as IDs. Unsupported files such as `.import`
sidecars are ignored. Only put
assets you intend to distribute in this folder: they ship in both editor and game
binaries, whether or not a particular game uses them.

## Select a texture in a game

Create **New SharedTexture** in a texture resource field, then expand the resource
and open **Asset**. The searchable menu shows folders as nested submenus and each
image with a thumbnail. Choose **Clear** to remove the selection. The outer
resource menu remains the normal Godot resource menu.

Save the resource as `.tres` to reuse it across nodes. The saved ID is the relative
asset path, for example `examples/checker.svg`. The Inspector only exposes the
picker; there is no text field for entering paths. Replacing or editing an image
at the same path preserves references. Moving or renaming an image changes its ID.

Search matches a case-insensitive substring of the entire relative path. While
searching, matching images are shown as a flat list with thumbnails and their full
paths. Clear the search to restore the folder view. **Clear** removes the selected
asset and supports the Inspector's normal undo/redo behavior.

## Asset manifest

SCons generates `embedded_textures.gen.json` alongside the embedded C++ header
before compiling the module. It is a build output, ignored by Git, and is never
manually edited or committed. Adding or removing images updates it automatically.
You can also run
`python modules/shared_textures/texture_manifest.py` before building. IDs are paths,
so regeneration or replacing an image at the same path does not change its ID.
The manifest is a generated catalog of the current image paths; it contains no
format version, compatibility aliases, or records for deleted images.

Full-size textures are decoded on selection and cached by ID until engine
shutdown. Multiple resources with the same ID share one GPU texture. The editor
creates separate small thumbnails without populating that full-size GPU cache;
opening a menu can temporarily decode images on the CPU.

GDScript:

```gdscript
var texture := SharedTexture.new()
texture.asset = "examples/checker.svg"
$TextureRect.texture = texture
print(SharedTexture.get_available_assets())
```

C# with the matching fork-generated GodotSharp assemblies:

```csharp
var texture = new SharedTexture { Asset = "examples/checker.svg" };
GetNode<TextureRect>("TextureRect").Texture = texture;
GD.Print(SharedTexture.GetAvailableAssets());
```

Games must use this fork's export templates; stock Godot does not know the
`SharedTexture` resource type. Embedding is a distribution convenience, not asset
encryption. To omit this feature entirely, build with
`module_shared_textures_enabled=no`.

## Verification

From the repository root, run the build-generator tests:

```sh
python -m unittest discover -s modules/shared_textures/tests -p "test_*.py"
```

The runtime smoke test checks selection, decoding, shared GPU resources,
Inspector hints, change notifications, clearing, missing assets, and `.tres`
serialization. Run it with an editor built from this branch:

```sh
bin/godot.windows.editor.double.x86_64.mono.console.exe --headless --path modules/shared_textures/tests/smoke --script test_shared_texture.gd
```

The editor regression project checks folder search, thumbnails, and native keyboard
activation of Clear, then exits automatically:

```sh
bin/godot.windows.editor.double.x86_64.mono.console.exe --headless --editor --path modules/shared_textures/tests/picker --quit-after 300
```
