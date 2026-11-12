@tool
extends EditorPlugin

var failures := 0


func _enter_tree() -> void:
	run.call_deferred()


func check(condition: bool, message: String) -> void:
	if not condition:
		push_error(message)
		failures += 1


func find_node(node: Node, predicate: Callable) -> Node:
	if predicate.call(node):
		return node
	for child in node.get_children(true):
		var found := find_node(child, predicate)
		if found != null:
			return found
	return null


func run() -> void:
	await get_tree().process_frame
	var texture := SharedTexture.new()
	texture.asset = "examples/checker.svg"
	EditorInterface.edit_resource(texture)
	var property: EditorProperty
	for frame in 30:
		await get_tree().process_frame
		property = find_node(EditorInterface.get_inspector(), func(node): return node is EditorProperty and node.get_edited_property() == "asset") as EditorProperty
		if property != null:
			break
	check(property != null, "Asset property editor exists")
	if property != null:
		var button := find_node(property, func(node): return node is MenuButton) as MenuButton
		check(button != null, "Asset uses the custom picker")
		if button != null:
			var popup := button.get_popup()
			popup.popup()
			var search := find_node(popup, func(node): return node is LineEdit) as LineEdit
			search.text = "EXAMPLES"
			search.text_changed.emit(search.text)
			var matches := 0
			for i in popup.item_count:
				if popup.get_item_text(i) == "examples/checker.svg":
					matches += 1
					check(popup.get_item_submenu_node(i) == null, "Search returns images directly")
					check(popup.get_item_icon(i) != null, "Search result has a thumbnail")
			check(matches == 1, "Case-insensitive full-path search finds the image")
			search.text = ""
			search.text_changed.emit(search.text)
			check(popup.get_item_submenu_node(2) != null, "Empty query restores folders")
			popup.set_focused_item(0)
			var enter := InputEventKey.new()
			enter.keycode = KEY_ENTER
			enter.pressed = true
			Input.parse_input_event(enter)
			await get_tree().process_frame
			check(texture.asset.is_empty(), "Activating Clear removes the selection")
			popup.hide()
	print("SharedTexture picker regression: ", "passed" if failures == 0 else "FAILED")
	get_tree().quit(0 if failures == 0 else 1)
