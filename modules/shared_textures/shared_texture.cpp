/**************************************************************************/
/*  shared_texture.cpp                                                    */
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

#include "shared_texture.h"

#include "embedded_textures.gen.h"

#include "core/object/class_db.h"
#include "core/os/mutex.h"

static HashMap<String, Ref<ImageTexture>> shared_texture_cache;
static Mutex shared_texture_cache_mutex;

static Ref<Image> decode_shared_image(const EmbeddedTextureEntry *p_entry) {
	Vector<uint8_t> buffer;
	buffer.resize(p_entry->size);
	memcpy(buffer.ptrw(), p_entry->data, p_entry->size);
	Ref<Image> image;
	image.instantiate();
	const String extension = String(p_entry->name).get_extension();
	Error error = ERR_FILE_UNRECOGNIZED;
	if (extension == "png") {
		error = image->load_png_from_buffer(buffer);
	} else if (extension == "svg") {
		error = image->load_svg_from_buffer(buffer);
	} else if (extension == "jpg" || extension == "jpeg") {
		error = image->load_jpg_from_buffer(buffer);
	} else if (extension == "webp") {
		error = image->load_webp_from_buffer(buffer);
	}
	return error == OK ? image : Ref<Image>();
}

void SharedTexture::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_asset", "asset"), &SharedTexture::set_asset);
	ClassDB::bind_method(D_METHOD("get_asset"), &SharedTexture::get_asset);
	ClassDB::bind_static_method("SharedTexture", D_METHOD("get_available_assets"), &SharedTexture::get_available_assets);
	ClassDB::bind_static_method("SharedTexture", D_METHOD("get_asset_id", "path_or_id"), &SharedTexture::get_asset_id);
	ClassDB::bind_static_method("SharedTexture", D_METHOD("get_asset_path", "id"), &SharedTexture::get_asset_path);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "asset"), "set_asset", "get_asset");
}

PackedStringArray SharedTexture::get_available_assets() {
	PackedStringArray names;
	for (int i = 0; i < embedded_texture_count; i++) {
		names.push_back(String::utf8(embedded_textures[i].name));
	}
	return names;
}

Ref<Image> SharedTexture::get_asset_preview_image(const String &p_value) {
	const String p_id = get_asset_id(p_value);
	for (int i = 0; i < embedded_texture_count; i++) {
		if (p_id == embedded_textures[i].id || p_id == embedded_textures[i].name) {
			return decode_shared_image(&embedded_textures[i]);
		}
	}
	return Ref<Image>();
}

String SharedTexture::get_asset_id(const String &p_path_or_id) {
	for (int i = 0; i < embedded_texture_count; i++) {
		if (p_path_or_id == embedded_textures[i].name || p_path_or_id == embedded_textures[i].id) {
			return String::utf8(embedded_textures[i].id);
		}
	}
	return p_path_or_id;
}

String SharedTexture::get_asset_path(const String &p_id) {
	for (int i = 0; i < embedded_texture_count; i++) {
		if (p_id == embedded_textures[i].id || p_id == embedded_textures[i].name) {
			return String::utf8(embedded_textures[i].name);
		}
	}
	return p_id;
}

void SharedTexture::set_asset(const String &p_asset) {
	const String identifier = get_asset_id(p_asset);
	if (asset == identifier) {
		return;
	}
	asset = identifier;
	texture.unref();
	if (!asset.is_empty()) {
		MutexLock lock(shared_texture_cache_mutex);
		const Ref<ImageTexture> *cached = shared_texture_cache.getptr(asset);
		if (cached) {
			texture = *cached;
		} else {
			const EmbeddedTextureEntry *entry = nullptr;
			for (int i = 0; i < embedded_texture_count; i++) {
				if (asset == embedded_textures[i].id) {
					entry = &embedded_textures[i];
					break;
				}
			}
			if (entry) {
				Ref<Image> image = decode_shared_image(entry);
				if (image.is_valid() && !image->is_empty()) {
					texture = ImageTexture::create_from_image(image);
					shared_texture_cache.insert(asset, texture);
				} else {
					WARN_PRINT(vformat("Cannot decode shared texture '%s'. Check the image and whether its decoder is enabled in this engine build.", asset));
				}
			} else {
				WARN_PRINT(vformat("Shared texture '%s' is not embedded in this engine build.", asset));
			}
		}
	}
	notify_property_list_changed();
	emit_changed();
}

String SharedTexture::get_asset() const {
	return asset;
}

void SharedTexture::clear_cache() {
	MutexLock lock(shared_texture_cache_mutex);
	shared_texture_cache.clear();
}

int SharedTexture::get_width() const {
	return texture.is_valid() ? texture->get_width() : 0;
}

int SharedTexture::get_height() const {
	return texture.is_valid() ? texture->get_height() : 0;
}

Image::Format SharedTexture::get_format() const {
	return texture.is_valid() ? texture->get_format() : Image::FORMAT_RGBA8;
}

int SharedTexture::get_mipmap_count() const {
	return texture.is_valid() ? texture->get_mipmap_count() : 0;
}

bool SharedTexture::has_mipmaps() const {
	return texture.is_valid() && texture->has_mipmaps();
}

bool SharedTexture::has_alpha() const {
	return texture.is_valid() && texture->has_alpha();
}

bool SharedTexture::is_pixel_opaque(int p_x, int p_y) const {
	return texture.is_valid() && texture->is_pixel_opaque(p_x, p_y);
}

RID SharedTexture::get_rid() const {
	return texture.is_valid() ? texture->get_rid() : RID();
}

Ref<Image> SharedTexture::get_image() const {
	return texture.is_valid() ? texture->get_image() : Ref<Image>();
}

void SharedTexture::draw(RID p_canvas_item, const Point2 &p_pos, const Color &p_modulate, bool p_transpose) const {
	if (texture.is_valid()) {
		texture->draw(p_canvas_item, p_pos, p_modulate, p_transpose);
	}
}

void SharedTexture::draw_rect(RID p_canvas_item, const Rect2 &p_rect, bool p_tile, const Color &p_modulate, bool p_transpose) const {
	if (texture.is_valid()) {
		texture->draw_rect(p_canvas_item, p_rect, p_tile, p_modulate, p_transpose);
	}
}

void SharedTexture::draw_rect_region(RID p_canvas_item, const Rect2 &p_rect, const Rect2 &p_src_rect, const Color &p_modulate, bool p_transpose, bool p_clip_uv) const {
	if (texture.is_valid()) {
		texture->draw_rect_region(p_canvas_item, p_rect, p_src_rect, p_modulate, p_transpose, p_clip_uv);
	}
}
