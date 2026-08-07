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
#ifndef THEME_H
#define THEME_H

#include <array>
#include <memory>
#include <string>
#include <string_view>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "../base/CVarRegistry.h"
#include "../cvar/CVarColor.h"
#include "../cvar/CVarInt.h"
#include "../renderer/AtlasArray.h"
#include "../renderer/AtlasEntry.h"
#include "../renderer/QuadTexture.h"
#include "../highlighter/TokenId.h"
#include "ColorId.h"
#include "DimensionId.h"


/**
 * @brief Manages all UI theme assets, including fonts, colors, and dimensions.
 *
 * This class encapsulates font loading via FreeType, color and dimension configuration via CVars,
 * and provides glyph rendering information via a glyph atlas. It also integrates with a
 * CVarRegistry to expose runtime-modifiable theme variables.
 */
class Theme final {
public:
    /** @brief Default font file name expected in the theme folder. */
    static constexpr auto FONT_FILE = "font.ttf";

    /** @brief Default font size in pixels. */
    static constexpr auto DEFAULT_FONT_SIZE = 16;

    /** @brief Minimum font size allowed. */
    static constexpr auto MIN_FONT_SIZE = 13;

    /** @brief Maximum font size allowed. */
    static constexpr auto MAX_FONT_SIZE = 96;


private:
    /** Handle to the FreeType library instance. */
    FT_Library m_ft_library;

    /** Handle to the font face used for rendering. */
    FT_Face m_font;

    /** Color CVars for ui rendering, indexed by ColorId. */
    std::array<std::shared_ptr<CVarColor>, COLOR_ID_COUNT> m_colors;

    /** Color CVars for text highlight rendering, indexed by TokenId. */
    std::array<std::shared_ptr<CVarColor>, TOKEN_ID_COUNT> m_highlight_colors;

    /** Dimension CVars used for rendering, indexed by DimensionId. */
    std::array<std::shared_ptr<CVarInt>, DIMENSION_ID_COUNT> m_dimensions;

    /** Atlas array storing glyph and sprite metadata. */
    AtlasArray m_atlas_array;

    /** Texture storing glyph pixel data. */
    QuadTexture m_quad_texture;

    /** Font size CVar. */
    std::shared_ptr<CVarInt> m_font_size;

    /** Effective font size ceiling, derived from the loaded face and never above MAX_FONT_SIZE. */
    int32_t m_max_font_size;

    /** Height of a line in pixels. */
    int32_t m_line_height;

    /** Horizontal advance per glyph. */
    int32_t m_font_advance;

    /** Vertical descender below the baseline. */
    int32_t m_font_descender;

private:
    /**
     * @brief Derives the largest font size whose glyphs still fit the atlas texture.
     *
     * Uses the design bbox of the loaded face, so it must be called once the face is loaded and
     * before the first size request. Falls back to MAX_FONT_SIZE when the face gives nothing usable.
     */
    void computeMaxFontSize();

    /** @brief Registers all UI color CVars with the command manager. */
    void registerThemeColorCVar(CVarRegistry &registry);

    /** @brief Registers all syntax highlight color CVars. */
    void registerHighLightColorCVar(CVarRegistry &registry);

    /** @brief Registers dimension CVars used for layout and spacing. */
    void registerThemeDimensionCVar(CVarRegistry &registry);

public:
    /** @brief Deleted copy constructor. */
    Theme(const Theme &) = delete;

    /** @brief Deleted copy assignment operator. */
    Theme &operator=(const Theme &) = delete;

    /** @brief Constructs a Theme instance. */
    explicit Theme();

    /**
     * @brief Initializes the Theme system.
     *
     * Loads the font, registers theme-related CVars, and prepares rendering assets.
     *
     * @param registry The CVarRegistry to register CVars with.
     * @param path Filesystem path to the theme folder (must contain FONT_FILE).
     */
    void create(CVarRegistry &registry, std::string_view path);

    /** @brief Releases all internal resources. */
    void destroy();

    /**
     * @brief Sets the font size used for rendering text.
     *
     * @param size Font size in pixels.
     */
    void setFontSize(int32_t size);

    /** @brief Returns the current font size in pixels. */
    int32_t getFontSize() const;

    /**
     * @brief Retrieves a color value from the theme.
     *
     * @param id Identifier of the color.
     * @return The associated ThemeColor.
     */
    [[nodiscard]] const Color &getColor(ColorId id) const;

    /**
     * @brief Retrieves a syntax highlight color by token type.
     *
     * @param id The token identifier.
     * @return Reference to the color.
     */
    [[nodiscard]] const Color &getColor(TokenId id) const;

    /**
     * @brief Returns glyph metadata for the given character.
     *
     * @param character The UTF-16 character.
     * @return Reference to the glyph's atlas entry.
     */
    [[nodiscard]] const AtlasEntry &getCharacter(char16_t character);

    /**
     * @brief Retrieves a dimension value by its identifier.
     *
     * @param id The dimension identifier.
     * @return Dimension value in pixels.
     */
    [[nodiscard]] int32_t getDimension(DimensionId id) const;

    /** @brief Returns the height of a text line in pixels. */
    [[nodiscard]] int32_t getLineHeight() const;

    /** @brief Returns the horizontal advance (spacing) between glyphs. */
    [[nodiscard]] int32_t getFontAdvance() const;

    /** @brief Returns the font descender (used for baseline alignment). */
    [[nodiscard]] int32_t getFontDescender() const;

    /**
     * @brief Calculates the rendered width of a UTF-16 string, one font advance per unit.
     *
     * Tabs get no special width: every caller measures tab-free text. Buffer text, where tabs
     * snap to tab stops, is measured by the views instead. The width lives in content space: a
     * buffer line can be long enough for it to overflow a 32-bit pixel count, so the measure is
     * carried in 64 bits end to end.
     *
     * @param text Text to measure.
     * @return Width in pixels.
     */
    [[nodiscard]] int64_t measure(std::u16string_view text) const;

    /**
     * @brief Truncates a UTF-16 string from the start so it fits the given width.
     *
     * When the text is too wide, the leading part is replaced by an ellipsis (U+2026).
     * Tabs are ignored, matching measure().
     *
     * @param text Text to truncate.
     * @param maxWidth Maximum width in pixels.
     * @return The text unchanged if it fits, otherwise the ellipsized tail.
     */
    [[nodiscard]] std::u16string ellipsizeStart(std::u16string_view text, int32_t maxWidth) const;
};


#endif //THEME_H
