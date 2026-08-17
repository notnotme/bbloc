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
      m_label_font(nullptr),
      m_font_size(std::make_shared<CVarInt>(0)),
      m_max_font_size(MAX_FONT_SIZE),
      m_line_height(0),
      m_font_advance(0),
      m_font_descender(0),
      m_label_line_height(0),
      m_label_advance(0),
      m_label_descender(0) {}

void Theme::create(CVarRegistry &registry, const std::string_view path) {
    // Create the atlas and texture
    m_atlas_array.create(UINT8_MAX);
    m_quad_texture.create(0, UINT8_MAX);

    // Set up the FT library and load theme text font
    if (FT_Init_FreeType(&m_ft_library) != FT_Err_Ok) {
        throw std::runtime_error("Theme::create: FT_Init_FreeType failed.");
    }

    const auto font_file_path = std::string(path).append(FONT_FILE);
    if (FT_New_Face(m_ft_library, font_file_path.data(), 0, &m_font) != 0) {
        throw std::runtime_error(std::string("Theme::create: FT_New_Face failed: ").append(FONT_FILE));
    }

    if (!FT_IS_FIXED_WIDTH(m_font)) {
        // We need a fixed width font
        throw std::runtime_error("Theme::create: Font is not fixed width.");
    }

    // The size ceiling depends on the face, derive it before requesting any size.
    computeMaxFontSize();

    setFontSize(DEFAULT_FONT_SIZE);

    // The OSK key labels render from their own face on the same file, sized once and never
    // touched again, so they keep a fixed size while dim_font_size moves the main face.
    m_label_atlas.create(LABEL_LAYER_COUNT);
    m_label_texture.create(LABEL_TEXTURE_UNIT, LABEL_LAYER_COUNT);
    if (FT_New_Face(m_ft_library, font_file_path.data(), 0, &m_label_font) != 0) {
        throw std::runtime_error(std::string("Theme::create: FT_New_Face failed: ").append(FONT_FILE));
    }

    if (!requestNominalSize(m_label_font, std::min(LABEL_FONT_SIZE, m_max_font_size))) {
        throw std::runtime_error("Theme::create: FT_Request_Size failed on the label face.");
    }

    readFaceMetrics(m_label_font, m_label_line_height, m_label_advance, m_label_descender);

    registerThemeColorCVar(registry);
    registerHighLightColorCVar(registry);
    registerThemeDimensionCVar(registry);
}

void Theme::destroy() {
    // Destroy texture and font
    m_quad_texture.destroy();
    m_label_texture.destroy();

    // Clear Freetype
    FT_Done_Face(m_font);
    FT_Done_Face(m_label_font);
    FT_Done_FreeType(m_ft_library);

    // Clear data
    m_colors.fill(nullptr);
    m_highlight_colors.fill(nullptr);
    m_dimensions.fill(nullptr);
    m_atlas_array.destroy();
    m_label_atlas.destroy();

    // Default states
    m_ft_library = nullptr;
    m_font = nullptr;
    m_label_font = nullptr;
    m_max_font_size = MAX_FONT_SIZE;
    m_line_height = 0;
    m_font_advance = 0;
    m_font_descender = 0;
    m_label_line_height = 0;
    m_label_advance = 0;
    m_label_descender = 0;
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

    // The size is requested as a nominal size at 96 DPI (see requestNominalSize), so convert the
    // pixel-per-EM bound back into the size unit the request takes.
    const auto max_ppem = UINT8_MAX * static_cast<int64_t>(m_font->units_per_EM) / bbox_height;
    const auto max_size = static_cast<int32_t>(std::min<int64_t>(max_ppem * 72 / 96, MAX_FONT_SIZE));

    // A font shipping an oversized bbox must not push the cap below the minimum size,
    // std::clamp requires a valid interval. The per-glyph fallback covers what is left.
    m_max_font_size = std::max(max_size, MIN_FONT_SIZE);
}

void Theme::registerThemeColorCVar(CVarRegistry &registry) {
    // Create default colors for the theme
    const auto &cvar_margin_background_color         = m_colors[static_cast<size_t>(ColorId::MarginBackground)]       = std::make_shared<CVarColor>(220, 220, 220, 255);
    const auto &cvar_info_bar_background_color       = m_colors[static_cast<size_t>(ColorId::InfoBarBackground)]      = std::make_shared<CVarColor>(210, 210, 210, 255);
    const auto &cvar_editor_background_color         = m_colors[static_cast<size_t>(ColorId::EditorBackground)]       = std::make_shared<CVarColor>(250, 250, 250, 255);
    const auto &cvar_prompt_background_color         = m_colors[static_cast<size_t>(ColorId::PromptBackground)]       = std::make_shared<CVarColor>(210, 210, 210, 255);
    const auto &cvar_current_line_background_color   = m_colors[static_cast<size_t>(ColorId::LineBackground)]         = std::make_shared<CVarColor>(  0,   0,   0,  12);
    const auto &cvar_selected_text_background_color  = m_colors[static_cast<size_t>(ColorId::SelectedTextBackground)] = std::make_shared<CVarColor>(  0, 200, 255,  32);
    const auto &cvar_line_number_color               = m_colors[static_cast<size_t>(ColorId::LineNumber)]             = std::make_shared<CVarColor>(  0,   0,   0, 220);
    const auto &cvar_info_bar_text_color             = m_colors[static_cast<size_t>(ColorId::InfoBarText)]            = std::make_shared<CVarColor>(  0,   0,   0, 220);
    const auto &cvar_prompt_text_color               = m_colors[static_cast<size_t>(ColorId::PromptText)]             = std::make_shared<CVarColor>(  0,   0,   0, 220);
    const auto &cvar_prompt_input_text_color         = m_colors[static_cast<size_t>(ColorId::PromptInputText)]        = std::make_shared<CVarColor>(  0,   0,   0, 220);
    const auto &cvar_border_color                    = m_colors[static_cast<size_t>(ColorId::Border)]                 = std::make_shared<CVarColor>(150, 150, 150, 255);
    const auto &cvar_cursor_indicator_color          = m_colors[static_cast<size_t>(ColorId::CursorIndicator)]        = std::make_shared<CVarColor>(  0,   0,   0, 255);
    const auto &cvar_scrollbar_background_color      = m_colors[static_cast<size_t>(ColorId::ScrollbarBackground)]    = std::make_shared<CVarColor>(225, 225, 225, 255);
    const auto &cvar_scrollbar_thumb_color           = m_colors[static_cast<size_t>(ColorId::ScrollbarThumb)]         = std::make_shared<CVarColor>(150, 150, 150, 255);
    const auto &cvar_osk_background_color            = m_colors[static_cast<size_t>(ColorId::OskBackground)]          = std::make_shared<CVarColor>(210, 210, 210, 255);
    const auto &cvar_osk_key_background_color        = m_colors[static_cast<size_t>(ColorId::OskKeyBackground)]       = std::make_shared<CVarColor>(240, 240, 240, 255);
    const auto &cvar_osk_key_text_color              = m_colors[static_cast<size_t>(ColorId::OskKeyText)]             = std::make_shared<CVarColor>(  0,   0,   0, 220);
    const auto &cvar_osk_key_cursor_color            = m_colors[static_cast<size_t>(ColorId::OskKeyCursor)]          = std::make_shared<CVarColor>(  0, 200, 255,  96);
    const auto &cvar_osk_key_pressed_color           = m_colors[static_cast<size_t>(ColorId::OskKeyPressed)]         = std::make_shared<CVarColor>(200, 205, 215, 255);

    // Make colors accessible from the console
    registry.registerCvar(u"col_margin_background",        cvar_margin_background_color, nullptr);
    registry.registerCvar(u"col_info_bar_background",      cvar_info_bar_background_color, nullptr);
    registry.registerCvar(u"col_editor_background",        cvar_editor_background_color, nullptr);
    registry.registerCvar(u"col_prompt_background",        cvar_prompt_background_color, nullptr);
    registry.registerCvar(u"col_current_line_background",  cvar_current_line_background_color, nullptr);
    registry.registerCvar(u"col_selected_text_background", cvar_selected_text_background_color, nullptr);
    registry.registerCvar(u"col_line_number",              cvar_line_number_color, nullptr);
    registry.registerCvar(u"col_info_bar_text",            cvar_info_bar_text_color, nullptr);
    registry.registerCvar(u"col_prompt_text",              cvar_prompt_text_color, nullptr);
    registry.registerCvar(u"col_prompt_input_text",        cvar_prompt_input_text_color, nullptr);
    registry.registerCvar(u"col_border",                   cvar_border_color, nullptr);
    registry.registerCvar(u"col_cursor_indicator",         cvar_cursor_indicator_color, nullptr);
    registry.registerCvar(u"col_scrollbar",                cvar_scrollbar_background_color, nullptr);
    registry.registerCvar(u"col_scrollbar_thumb",          cvar_scrollbar_thumb_color, nullptr);
    registry.registerCvar(u"col_osk_background",           cvar_osk_background_color, nullptr);
    registry.registerCvar(u"col_osk_key_background",       cvar_osk_key_background_color, nullptr);
    registry.registerCvar(u"col_osk_key_text",             cvar_osk_key_text_color, nullptr);
    registry.registerCvar(u"col_osk_key_cursor",           cvar_osk_key_cursor_color, nullptr);
    registry.registerCvar(u"col_osk_key_pressed",          cvar_osk_key_pressed_color, nullptr);
}

void Theme::registerHighLightColorCVar(CVarRegistry &registry) {
    // Create default highlight colors
    const auto &cvar_hl_text_color           = m_highlight_colors[static_cast<size_t>(TokenId::None)]         = std::make_shared<CVarColor>( 64,  64,  64, 255);
    const auto &cvar_hl_comment_color        = m_highlight_colors[static_cast<size_t>(TokenId::Comment)]      = std::make_shared<CVarColor>(160, 160, 160, 200);
    const auto &cvar_hl_string_color         = m_highlight_colors[static_cast<size_t>(TokenId::String)]       = std::make_shared<CVarColor>(  0, 150,   0, 255);
    const auto &cvar_hl_preprocessor_color   = m_highlight_colors[static_cast<size_t>(TokenId::Preprocessor)] = std::make_shared<CVarColor>(150, 150,  64, 255);
    const auto &cvar_hl_number_color         = m_highlight_colors[static_cast<size_t>(TokenId::Number)]       = std::make_shared<CVarColor>(  0, 200, 200, 255);
    const auto &cvar_hl_keyword_color        = m_highlight_colors[static_cast<size_t>(TokenId::Keyword)]      = std::make_shared<CVarColor>(  0,   0, 200, 255);
    const auto &cvar_hl_statement_color      = m_highlight_colors[static_cast<size_t>(TokenId::Statement)]    = std::make_shared<CVarColor>(200,   0, 200, 255);
    const auto &cvar_hl_type_color           = m_highlight_colors[static_cast<size_t>(TokenId::Type)]         = std::make_shared<CVarColor>(  0, 128, 128, 255);
    const auto &cvar_hl_constant_color       = m_highlight_colors[static_cast<size_t>(TokenId::Constant)]     = std::make_shared<CVarColor>(128,  64,   0, 255);
    const auto &cvar_hl_function_color       = m_highlight_colors[static_cast<size_t>(TokenId::Function)]     = std::make_shared<CVarColor>(150, 100,  40, 255);
    const auto &cvar_hl_variable_color       = m_highlight_colors[static_cast<size_t>(TokenId::Variable)]     = std::make_shared<CVarColor>( 90,  90, 110, 255);

    // Make highlight colors accessible from the console
    registry.registerCvar(u"hl_text",          cvar_hl_text_color, nullptr);
    registry.registerCvar(u"hl_comment",       cvar_hl_comment_color, nullptr);
    registry.registerCvar(u"hl_string",        cvar_hl_string_color, nullptr);
    registry.registerCvar(u"hl_preprocessor",  cvar_hl_preprocessor_color, nullptr);
    registry.registerCvar(u"hl_number",        cvar_hl_number_color, nullptr);
    registry.registerCvar(u"hl_keyword",       cvar_hl_keyword_color, nullptr);
    registry.registerCvar(u"hl_statement",     cvar_hl_statement_color, nullptr);
    registry.registerCvar(u"hl_type",          cvar_hl_type_color, nullptr);
    registry.registerCvar(u"hl_constant",      cvar_hl_constant_color, nullptr);
    registry.registerCvar(u"hl_function",      cvar_hl_function_color, nullptr);
    registry.registerCvar(u"hl_variable",      cvar_hl_variable_color, nullptr);
}

void Theme::registerThemeDimensionCVar(CVarRegistry &registry) {
    // Create default dimensions for the theme
    const auto &cvar_padding_width   = m_dimensions[static_cast<size_t>(DimensionId::PaddingWidth)]   = std::make_shared<CVarInt>( 8);
    const auto &cvar_indicator_width = m_dimensions[static_cast<size_t>(DimensionId::IndicatorWidth)] = std::make_shared<CVarInt>( 2);
    const auto &cvar_border_size     = m_dimensions[static_cast<size_t>(DimensionId::BorderSize)]     = std::make_shared<CVarInt>( 1);
    const auto &cvar_tab_to_space    = m_dimensions[static_cast<size_t>(DimensionId::TabToSpace)]     = std::make_shared<CVarInt>( 4);
    const auto &cvar_page_up_down    = m_dimensions[static_cast<size_t>(DimensionId::PageUpDown)]     = std::make_shared<CVarInt>(10);
    const auto &cvar_scrollbar_width = m_dimensions[static_cast<size_t>(DimensionId::ScrollbarWidth)] = std::make_shared<CVarInt>(10);
    const auto &cvar_osk_height      = m_dimensions[static_cast<size_t>(DimensionId::OskHeight)]      = std::make_shared<CVarInt>(40);
    const auto &cvar_osk_key_gap     = m_dimensions[static_cast<size_t>(DimensionId::OskKeyGap)]      = std::make_shared<CVarInt>( 1);

    // Make dimensions accessible from the console
    registry.registerCvar(u"dim_padding_width",    cvar_padding_width, nullptr);
    registry.registerCvar(u"dim_indicator_width",  cvar_indicator_width, nullptr);
    registry.registerCvar(u"dim_border_size",      cvar_border_size, nullptr);
    registry.registerCvar(u"dim_tab_to_space",     cvar_tab_to_space, nullptr);
    registry.registerCvar(u"dim_page_up_down",     cvar_page_up_down, nullptr);
    registry.registerCvar(u"dim_scrollbar_width",  cvar_scrollbar_width, nullptr);
    registry.registerCvar(u"dim_osk_height",       cvar_osk_height, nullptr);
    registry.registerCvar(u"dim_osk_key_gap",      cvar_osk_key_gap, nullptr);

    // Register a cvar to change the font size. It needs a callback.
    registry.registerCvar(u"dim_font_size", m_font_size, [&]{ setFontSize(m_font_size->m_value); });
}

void Theme::setFontSize(int32_t size) {
    size = std::clamp(size, MIN_FONT_SIZE, m_max_font_size);
    if (!requestNominalSize(m_font, size)) {
        if (m_line_height == 0) {
            // Nothing was ever sized successfully: there are no previous metrics to keep, and a
            // zero line height divides by zero as soon as a view renders.
            throw std::runtime_error("Theme::setFontSize: FT_Request_Size failed.");
        }

        // Keep the previous size and metrics rather than deriving them from a failed request
        return;
    }

    readFaceMetrics(m_font, m_line_height, m_font_advance, m_font_descender);
    m_font_size->m_value = size;
    m_atlas_array.clearCharacters();
}

bool Theme::requestNominalSize(const FT_Face face, const int32_t size) {
    FT_Size_RequestRec size_req = {
        .type = FT_SIZE_REQUEST_TYPE_NOMINAL,
        .width = 0,
        .height = size * 64,
        .horiResolution = 96,
        .vertResolution = 96
    };
    return FT_Request_Size(face, &size_req) == FT_Err_Ok;
}

void Theme::readFaceMetrics(const FT_Face face, int32_t &lineHeight, int32_t &fontAdvance, int32_t &fontDescender) {
    const auto bbox_y_max = FT_MulFix(face->bbox.yMax, face->size->metrics.y_scale) >> 6;
    const auto bbox_y_min = FT_MulFix(face->bbox.yMin, face->size->metrics.y_scale) >> 6;
    const auto font_height = static_cast<int32_t>(face->size->metrics.height >> 6);
    const auto bbox_max_height = static_cast<int32_t>(bbox_y_max - bbox_y_min);
    fontAdvance = static_cast<int32_t>(face->size->metrics.max_advance) >> 6;
    fontDescender = static_cast<int32_t>(face->size->metrics.descender >> 6) - (bbox_max_height - font_height) / 2;
    lineHeight = font_height + (bbox_max_height - font_height);
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

const AtlasEntry *Theme::loadGlyph(const FT_Face face, AtlasArray &atlas, QuadTexture &texture, const char16_t character) {
    // Stands in for a glyph the atlas cannot store: it draws nothing instead of aborting the frame.
    static constexpr auto blank_entry = AtlasEntry {};

    // If we already generated the character, we return it
    if (const auto &entry = atlas.get(character); entry != nullptr) {
        return entry;
    }

    // Generate a new character
    if(FT_Load_Char(face, character, FT_LOAD_RENDER) != FT_Err_Ok) {
        return nullptr;
    }

    // Insert the character into the atlas
    const auto atlas_entry = atlas.insert(
        character,
        face->glyph->bitmap.width,
        face->glyph->bitmap.rows,
        face->glyph->bitmap_left,
        face->glyph->bitmap_top);

    if (atlas_entry == nullptr) {
        // The glyph is bigger than the texture, or the atlas ran out of layers. Remember the
        // failure, otherwise every frame reloads the glyph for each of its occurrences on screen.
        atlas.insertBlank(character);
        return &blank_entry;
    }

    texture.blit(
        atlas_entry->texture_s,
        atlas_entry->texture_t,
        atlas_entry->width,
        atlas_entry->height,
        atlas_entry->layer,
        face->glyph->bitmap.buffer);

    return atlas_entry;
}

const AtlasEntry &Theme::getCharacter(const char16_t character) {
    const auto *entry = loadGlyph(m_font, m_atlas_array, m_quad_texture, character);
    if (entry == nullptr) {
        throw std::runtime_error("Theme::getCharacter FT_Load_Char failed");
    }

    return *entry;
}

const AtlasEntry &Theme::getLabelCharacter(const char16_t character) {
    const auto *entry = loadGlyph(m_label_font, m_label_atlas, m_label_texture, character);
    if (entry == nullptr) {
        throw std::runtime_error("Theme::getLabelCharacter FT_Load_Char failed");
    }

    return *entry;
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

int32_t Theme::getLabelLineHeight() const {
    return m_label_line_height;
}

int32_t Theme::getLabelAdvance() const {
    return m_label_advance;
}

int32_t Theme::getLabelDescender() const {
    return m_label_descender;
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
    if (tail_start < text.length() && (text[tail_start] & 0xFC00) == 0xDC00) {
        // Never start the tail on a low surrogate, drop it to keep the pair intact
        ++tail_start;
    }

    return std::u16string(1, u'…').append(text.substr(tail_start));
}
