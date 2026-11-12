/**************************************************************************/
/*  shared_texture_editor.cpp                                             */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "shared_texture_editor.h"

#ifdef TOOLS_ENABLED
#include "shared_texture.h"

#include "core/object/callable_mp.h"
#include "editor/inspector/editor_inspector.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/popup_menu.h"

class SharedTextureAssetProperty : public EditorProperty {
	GDCLASS(SharedTextureAssetProperty, EditorProperty);

	static constexpr int CLEAR_SELECTION = 0x7fffffff;
	MenuButton *button = nullptr;
	LineEdit *search = nullptr;
	PackedStringArray assets;
	Vector<Ref<Texture2D>> previews;

	Ref<Texture2D> preview(const String &p_path) {
		Ref<Image> image = SharedTexture::get_asset_preview_image(p_path);
		if (image.is_null() || image->is_empty()) {
			return Ref<Texture2D>();
		}
		const float ratio = 32.0 / MAX(image->get_width(), image->get_height());
		image->resize(MAX(1, int(image->get_width() * ratio)), MAX(1, int(image->get_height() * ratio)));
		return ImageTexture::create_from_image(image);
	}

	void selected(int p_index) {
		if (is_read_only()) {
			return;
		}
		if (p_index != CLEAR_SELECTION) {
			ERR_FAIL_INDEX(p_index, assets.size());
		}
		emit_changed(get_edited_property(), p_index == CLEAR_SELECTION ? String() : SharedTexture::get_asset_id(assets[p_index]));
	}

	void _set_read_only(bool p_read_only) override {
		button->set_disabled(p_read_only);
	}

	LineEdit *find_search(Node *p_node) {
		if (LineEdit *line = Object::cast_to<LineEdit>(p_node)) {
			return line;
		}
		for (int i = 0; i < p_node->get_child_count(true); i++) {
			if (LineEdit *line = find_search(p_node->get_child(i, true))) {
				return line;
			}
		}
		return nullptr;
	}

	void populate() {
		search->set_text("");
		filter("");
	}

	void filter(const String &p_query) {
		PopupMenu *root = button->get_popup();
		root->clear();
		root->add_item(TTR("Clear"), CLEAR_SELECTION);
		root->add_separator();
		HashMap<String, PopupMenu *> folders;
		folders.insert("", root);
		assets = SharedTexture::get_available_assets();
		if (previews.size() != assets.size()) {
			previews.resize(assets.size());
		}
		for (int i = 0; i < assets.size(); i++) {
			if (!p_query.is_empty() && assets[i].findn(p_query) < 0) {
				continue;
			}
			PopupMenu *asset_menu = root;
			String path;
			PackedStringArray parts = assets[i].split("/");
			for (int j = 0; p_query.is_empty() && j < parts.size() - 1; j++) {
				path = path.path_join(parts[j]);
				if (!folders.has(path)) {
					PopupMenu *folder = memnew(PopupMenu);
					folder->set_search_bar_enabled(false);
					folder->set_search_bar_min_item_count(0);
					folder->connect("id_pressed", callable_mp(this, &SharedTextureAssetProperty::selected));
					asset_menu->add_submenu_node_item(parts[j], folder);
					asset_menu->set_item_auto_translate_mode(-1, AUTO_TRANSLATE_MODE_DISABLED);
					folders.insert(path, folder);
				}
				asset_menu = folders[path];
			}
			if (previews[i].is_null()) {
				previews.write[i] = preview(assets[i]);
			}
			asset_menu->add_icon_item(previews[i], p_query.is_empty() ? parts[parts.size() - 1] : assets[i], i);
			asset_menu->set_item_auto_translate_mode(-1, AUTO_TRANSLATE_MODE_DISABLED);
			asset_menu->set_item_tooltip(-1, assets[i]);
		}
	}

public:
	void update_property() override {
		const String value = get_edited_object()->get(get_edited_property());
		const String path = SharedTexture::get_asset_path(value);
		button->set_text(value.is_empty() ? TTR("Select Texture") : path.get_file());
		button->set_tooltip_text(path);
		button->set_button_icon(value.is_empty() ? Ref<Texture2D>() : preview(value));
	}

	SharedTextureAssetProperty() {
		button = memnew(MenuButton);
		button->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
		button->set_clip_text(true);
		add_child(button);
		add_focusable(button);
		button->get_popup()->set_search_bar_enabled(true);
		button->get_popup()->set_search_bar_fuzzy_search_enabled(false);
		search = find_search(button->get_popup());
		search->connect("text_changed", callable_mp(this, &SharedTextureAssetProperty::filter));
		button->get_popup()->set_search_bar_min_item_count(0);
		button->get_popup()->connect("about_to_popup", callable_mp(this, &SharedTextureAssetProperty::populate));
		button->get_popup()->connect("id_pressed", callable_mp(this, &SharedTextureAssetProperty::selected));
	}
};

class SharedTextureInspectorPlugin : public EditorInspectorPlugin {
	GDCLASS(SharedTextureInspectorPlugin, EditorInspectorPlugin);

public:
	bool can_handle(Object *p_object) override {
		return Object::cast_to<SharedTexture>(p_object) != nullptr;
	}

	bool parse_property(Object *p_object, Variant::Type p_type, const String &p_path, PropertyHint p_hint, const String &p_hint_text, BitField<PropertyUsageFlags> p_usage, bool p_wide) override {
		if (p_path != "asset") {
			return false;
		}
		add_property_editor(p_path, memnew(SharedTextureAssetProperty));
		return true;
	}
};

SharedTextureEditorPlugin::SharedTextureEditorPlugin() {
	Ref<SharedTextureInspectorPlugin> inspector;
	inspector.instantiate();
	add_inspector_plugin(inspector);
}
#endif
