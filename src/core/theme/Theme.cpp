/*
* Copyright (C) 2026 Romain Graillot
 *
 * This file is part of bbloc.
 *
 * bbloc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bbloc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "Theme.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

#include <SDL.h>


Theme::Theme()
    : m_ft_library(nullptr),
      m_font(nullptr),
      m_font_size(std::make_shared<CVarInt>(0)),
      m_max_font_size(MAX_FONT_SIZE),
      m_line_height(0),
      m_font_advance(0),
      m_font_descender(0) {}

void Theme::destroy() {
    // Destroy texture and font
    m_quad_texture.destroy();

    // Clear Freetype
    FT_Done_Face(m_font);
    FT_Done_FreeType(m_ft_library);

    // Clear data
    m_colors.fill(nullptr);
    m_highlight_colors.fill(nullptr);
    m_dimensions.fill(nullptr);
    m_atlas_array.destroy();

    // Default states
    m_ft_library = nullptr;
    m_font = nullptr;
    m_max_font_size = MAX_FONT_SIZE;
    m_line_height = 0;
    m_font_advance = 0;
    m_font_descender = 0;
}

void Theme::computeMaxFontSize() {
    // The glyph bitmaps are stored in a UINT8_MAX tall texture, so a font size producing taller
    // bitmaps would make glyphs unrenderable. The design bbox bounds the tallest glyph of the
    // face, and it is known before any size request.
    m_max_font_size = MAX_FONT_SIZE;
    if (!FT_IS_SCALABLE(m_font)) {
        // units_per_EM and bbox are expressed in font units only for scalable faces
        return;
    }

    const auto bbox_height = static_cast<int64_t>(m_font->bbox.yMax) - static_cast<int64_t>(m_font->bbox.yMin);
    if (bbox_height <= 0 || m_font->units_per_EM == 0) {
        // Nothing usable to derive from, keep the static ceiling
        return;
    }

    // The size is requested as a nominal size at 96 DPI (see setFontSize), so convert the
    // pixel-per-EM bound back into the size unit the request takes.
    const auto max_ppem = UINT8_MAX * static_cast<int64_t>(m_font->units_per_EM) / bbox_height;
    const auto max_size = static_cast<int32_t>(std::min<int64_t>(max_ppem * 72 / 96, MAX_FONT_SIZE));

    // A font shipping an oversized bbox must not push the cap below the minimum size,
    // std::clamp requires a valid interval. The per-glyph fallback covers what is left.
    m_max_font_size = std::max(max_size, MIN_FONT_SIZE);
}

void Theme::setFontSize(int32_t size) {
    size = std::clamp(size, MIN_FONT_SIZE, m_max_font_size);
    FT_Size_RequestRec font_size_req = {
        .type = FT_SIZE_REQUEST_TYPE_NOMINAL,
        .width = 0,
        .height = size * 64,
        .horiResolution = 96,
        .vertResolution = 96
    };
    if (FT_Request_Size(m_font, &font_size_req) != FT_Err_Ok) {
        if (m_line_height == 0) {
            // Nothing was ever sized successfully: there are no previous metrics to keep, and a
            // zero line height divides by zero as soon as a view renders.
            throw std::runtime_error("Theme::setFontSize: FT_Request_Size failed.");
        }

        // Keep the previous size and metrics rather than deriving them from a failed request
        return;
    }

    const auto bbox_y_max = FT_MulFix(m_font->bbox.yMax, m_font->size->metrics.y_scale) >> 6;
    const auto bbox_y_min = FT_MulFix(m_font->bbox.yMin, m_font->size->metrics.y_scale) >> 6;
    const auto font_height = static_cast<int32_t>(m_font->size->metrics.height >> 6);
    const auto bbox_max_height = static_cast<int32_t>(bbox_y_max - bbox_y_min);
    m_font_advance = static_cast<int32_t>(m_font->size->metrics.max_advance) >> 6;
    m_font_descender = static_cast<int32_t>(m_font->size->metrics.descender >> 6) - (bbox_max_height - font_height) / 2;
    m_line_height = font_height + (bbox_max_height - font_height);
    m_font_size->m_value = size;
    m_atlas_array.clearCharacters();
}

int32_t Theme::getFontSize() const {
    return m_font_size->m_value;
}

const Color &Theme::getColor(const ColorId id) const {
    return m_colors[static_cast<size_t>(id)]->m_value;
}

const Color &Theme::getColor(const TokenId id) const {
    return m_highlight_colors[static_cast<size_t>(id)]->m_value;
}

const AtlasEntry &Theme::getCharacter(const char16_t character) {
    // Stands in for a glyph the atlas cannot store: it draws nothing instead of aborting the frame.
    static constexpr auto blank_entry = AtlasEntry {};

    // If we already generated the character, we return it
    if (const auto &entry = m_atlas_array.get(character); entry != nullptr) {
        return *entry;
    }

    // Generate a new character
    if(FT_Load_Char(m_font, character, FT_LOAD_RENDER) != FT_Err_Ok) {
        throw std::runtime_error("Theme::getCharacter FT_Load_Char failed");
    }

    // Insert the character into the atlas
    const auto atlas_entry = m_atlas_array.insert(
        character,
        m_font->glyph->bitmap.width,
        m_font->glyph->bitmap.rows,
        m_font->glyph->bitmap_left,
        m_font->glyph->bitmap_top);

    if (atlas_entry == nullptr) {
        // The glyph is bigger than the texture, or the atlas ran out of layers. Remember the
        // failure, otherwise every frame reloads the glyph for each of its occurrences on screen.
        m_atlas_array.insertBlank(character);
        return blank_entry;
    }

    m_quad_texture.blit(
        atlas_entry->texture_s,
        atlas_entry->texture_t,
        atlas_entry->width,
        atlas_entry->height,
        atlas_entry->layer,
        m_font->glyph->bitmap.buffer);

    return *atlas_entry;
}

int32_t Theme::getDimension(const DimensionId id) const {
    return m_dimensions[static_cast<size_t>(id)]->m_value;
}

int32_t Theme::getLineHeight() const {
    return m_line_height;
}

int32_t Theme::getFontAdvance() const {
    return m_font_advance;
}

int32_t Theme::getFontDescender() const {
    return m_font_descender;
}

int64_t Theme::measure(const std::u16string_view text) const {
    return static_cast<int64_t>(text.length()) * m_font_advance;
}

std::u16string Theme::ellipsizeStart(const std::u16string_view text, const int32_t maxWidth) const {
    if (measure(text) <= maxWidth) {
        return std::u16string(text);
    }

    const auto max_glyphs = maxWidth / m_font_advance;
    if (max_glyphs < 1) {
        return {};
    }

    auto tail_start = text.length() - static_cast<size_t>(max_glyphs - 1);
    if (tail_start < text.length() && text[tail_start] >= 0xDC00 && text[tail_start] <= 0xDFFF) {
        // Never start the tail on a low surrogate, drop it to keep the pair intact
        ++tail_start;
    }

    return std::u16string(1, u'…').append(text.substr(tail_start));
}
