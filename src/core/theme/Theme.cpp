#include "Theme.h"

#include <algorithm>
#include <stdexcept>

#include <SDL.h>


Theme::Theme()
    : m_ft_library(nullptr),
      m_font(nullptr),
      m_font_size(std::make_shared<CVarInt>(0)),
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
    m_colors.clear();
    m_atlas_array.destroy();

    // Default states
    m_ft_library = nullptr;
    m_font = nullptr;
    m_line_height = 0;
    m_font_advance = 0;
    m_font_descender = 0;
}

void Theme::setFontSize(int32_t size) {
    size = std::clamp(size, MIN_FONT_SIZE, MAX_FONT_SIZE);
    FT_Size_RequestRec font_size_req = {FT_SIZE_REQUEST_TYPE_NOMINAL, 0, size * 64, 96, 96};
    FT_Request_Size(m_font, &font_size_req);

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
    if (!m_colors.contains(id)) {
        throw std::runtime_error("Theme::getColor color does not exists.");
    }

    return m_colors.at(id)->m_value;
}

const Color &Theme::getColor(const TokenId id) const {
    if (!m_highlight_colors.contains(id)) {
        throw std::runtime_error("Theme::getColor color does not exists.");
    }

    return m_highlight_colors.at(id)->m_value;
}

const AtlasEntry &Theme::getCharacter(const char16_t character) {
    // If we already generated the character, we return it
    if (const auto &entry = m_atlas_array.get(character); entry != nullptr) {
        return *entry;
    }

    // Generate a new character
    if(FT_Load_Char(m_font, character, FT_LOAD_RENDER | FT_LOAD_DEFAULT) != FT_Err_Ok) {
        throw std::runtime_error("Theme::getCharacter FT_Load_Char failed");
    }

    // Insert the character into the atlas
    const auto &atlas_entry = m_atlas_array.insert(
        character,
        m_font->glyph->bitmap.width,
        m_font->glyph->bitmap.rows,
        static_cast<int8_t>(m_font->glyph->bitmap_left),
        static_cast<int8_t>(m_font->glyph->bitmap_top));

    m_quad_texture.blit(
        atlas_entry.texture_s,
        atlas_entry.texture_t,
        m_font->glyph->bitmap.width,
        m_font->glyph->bitmap.rows,
        atlas_entry.layer,
        m_font->glyph->bitmap.buffer);

    return atlas_entry;
}

int32_t Theme::getDimension(const DimensionId id) const {
    if (!m_dimensions.contains(id)) {
        throw std::runtime_error("Theme::getDimension dimension does not exists.");
    }

    return m_dimensions.at(id)->m_value;
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

int32_t Theme::measure(const std::u16string_view text, const bool ignoreTabs) {
    if (ignoreTabs) {
        return static_cast<int32_t>(text.length() * m_font_advance);
    }

    const auto tab_to_space = m_dimensions[DimensionId::TabToSpace];
    auto size = 0;
    for (const auto c : text) {
        size += c == '\t' ? m_font_advance * tab_to_space->m_value : m_font_advance;
    }
    return size;
}
