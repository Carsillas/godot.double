extends SceneTree

var failures: int = 0
var change_count: int = 0


func check(condition: bool, message: String) -> void:
	if not condition:
		push_error(message)
		failures += 1


func _initialize() -> void:
	var texture := SharedTexture.new()
	check(texture.asset.is_empty(), "New textures must have an empty selection")
	check(texture.get_size() == Vector2.ZERO, "Empty texture dimensions")
	check(texture.get_image() == null, "Empty texture image")
	check(SharedTexture.get_available_assets().has("examples/checker.svg"), "Sample is embedded")
	for asset in SharedTexture.get_available_assets():
		var embedded := SharedTexture.new()
		embedded.asset = asset
		check(embedded.get_width() > 0 and embedded.get_height() > 0, "Decode embedded asset: " + asset)
	texture.changed.connect(func(): change_count += 1)
	texture.asset = "examples/checker.svg"
	check(change_count == 1, "Selection emits changed")
	check(texture.get_size() == Vector2(32, 32), "SVG dimensions")
	var image := texture.get_image()
	check(image != null and not image.is_empty(), "SVG decoded")
	if image != null and not image.is_empty():
		check(image.get_pixel(4, 4) != image.get_pixel(20, 4), "Sample pixel colors")
	var other := SharedTexture.new()
	other.asset = texture.asset
	check(other.get_rid() == texture.get_rid(), "Same assets share a texture RID")
	check(texture.get_rid().is_valid(), "Selected texture has a valid RID")
	var canvas_item := RenderingServer.canvas_item_create()
	texture.draw(canvas_item, Vector2.ZERO)
	texture.draw_rect(canvas_item, Rect2(0, 0, 64, 64), true)
	texture.draw_rect_region(canvas_item, Rect2(0, 0, 16, 16), Rect2(0, 0, 16, 16))
	var rect := TextureRect.new()
	rect.texture = texture
	check(rect.texture == texture, "Usable as a normal Texture2D")
	rect.free()
	check(texture.asset == SharedTexture.get_asset_id("examples/checker.svg"), "Stable ID selection")
	check(texture.asset == "examples/checker.svg", "Selection stores the asset path")
	check(SharedTexture.get_asset_path(texture.asset) == "examples/checker.svg", "ID resolves to current path")
	var path := "user://shared_texture_smoke.tres"
	check(ResourceSaver.save(texture, path) == OK, "Save SharedTexture")
	var loaded := ResourceLoader.load(path, "", ResourceLoader.CACHE_MODE_IGNORE) as SharedTexture
	check(loaded != null and loaded.asset == texture.asset, "Round-trip selection")
	if loaded != null:
		check(loaded.get_size() == texture.get_size(), "Round-trip decoding")
	texture.asset = ""
	check(change_count == 2, "Clearing emits changed")
	check(texture.get_size() == Vector2.ZERO, "Clear dimensions")
	check(texture.get_image() == null, "Clear image")
	texture.draw(canvas_item, Vector2.ZERO)
	texture.draw_rect(canvas_item, Rect2(0, 0, 64, 64), true)
	texture.draw_rect_region(canvas_item, Rect2(0, 0, 16, 16), Rect2(0, 0, 16, 16))
	RenderingServer.free_rid(canvas_item)
	# This intentionally logs a warning; it must not erase the saved reference.
	texture.asset = "missing/example.png"
	check(texture.asset == "missing/example.png", "Missing ID retained")
	check(texture.get_image() == null, "Missing assets are empty")
	check(ResourceSaver.save(texture, path) == OK, "Save missing reference")
	loaded = ResourceLoader.load(path, "", ResourceLoader.CACHE_MODE_IGNORE) as SharedTexture
	check(loaded != null and loaded.asset == texture.asset, "Round-trip missing reference")
	DirAccess.remove_absolute(path)
	print("SharedTexture smoke test: ", "passed" if failures == 0 else "FAILED")
	quit(0 if failures == 0 else 1)
