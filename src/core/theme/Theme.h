#ifndef THEME_H
#define THEME_H

#include <memory>
#include <unordered_map>
#include <string>
#include <string_view>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "../renderer/AtlasArray.h"
#include "../renderer/AtlasEntry.h"
#include "../renderer/QuadTexture.h"
#include "../command/cvar/CVarColor.h"
#include "../command/cvar/CVarInt.h"
#include "../command/CommandManager.h"
#include "../highlight/TokenId.h"
#include "ColorId.h"
#include "DimensionId.h"


/**
 * @brief Manages all UI theme assets, including fonts, colors, and dimensions.
 *
 * This class encapsulates font loading via FreeType, color and dimension configuration via CVars,
 * and provides glyph rendering information via a glyph atlas. It also integrates with the
 * CommandManager to expose runtime-modifiable theme variables.
 */
class Theme final {
public:
    /** @brief Default font file name expected in the theme folder. */
    static constexpr auto FONT_FILE = "font.ttf";

    /** @brief Default font size in pixels. */
    static constexpr auto DEFAULT_FONT_SIZE = 25;

    /** @brief Minimum font size allowed. */
    static constexpr auto MIN_FONT_SIZE = 13;

    /** @brief Maximum font size allowed. */
    static constexpr auto MAX_FONT_SIZE = 127;


private:
    /** Handle to the FreeType library instance. */
    FT_Library m_ft_library;

    /** Handle to the font face used for rendering. */
    FT_Face m_font;

    /** Map storing color CVars for ui rendering. */
    std::unordered_map<ColorId, std::shared_ptr<CVarColor>> m_colors;

    /** Map storing color CVars for text highlight rendering. */
    std::unordered_map<TokenId, std::shared_ptr<CVarColor>> m_highlight_colors;

    /** Map storing dimension CVars used for rendering. */
    std::unordered_map<DimensionId, std::shared_ptr<CVarInt>> m_dimensions;

    /** Atlas array storing glyph and sprite metadata. */
    AtlasArray m_atlas_array;

    /** Texture storing glyph pixel data. */
    QuadTexture m_quad_texture;

    /** Font size CVar. */
    std::shared_ptr<CVarInt> m_font_size;

    /** Height of a line in pixels. */
    int32_t m_line_height;

    /** Horizontal advance per glyph. */
    int32_t m_font_advance;

    /** Vertical descender below the baseline. */
    int32_t m_font_descender;

private:
    /** @brief Registers all UI color CVars with the command manager. */
    void registerThemeColorCVar(CommandManager& commandManager);

    /** @brief Registers all syntax highlight color CVars. */
    void registerHighLightColorCVar(CommandManager& commandManager);

    /** @brief Registers dimension CVars used for layout and spacing. */
    void registerThemeDimensionCVar(CommandManager& commandManager);

public:
    /** @brief Deleted copy constructor. */
    Theme(const Theme &) = delete;

    /** @brief Deleted copy assignment operator. */
    Theme &operator=(const Theme &) = delete;

    /** @brief Constructs a Theme instance. */
    explicit Theme();

    /**
     * @brief Initializes the Theme system.
     * Loads the font, registers theme-related CVars, and prepares rendering assets.
     * @param commandManager The CommandManager to register CVars with.
     * @param path Filesystem path to the theme folder (must contain FONT_FILE).
     */
    void create(CommandManager& commandManager, std::string_view path);

    /** @brief Releases all internal resources. */
    void destroy();

    /**
     * @brief Sets the font size used for rendering text.
     * @param size Font size in pixels.
     */
    void setFontSize(int32_t size);

    /** @brief Returns the current font size in pixels. */
    int32_t getFontSize() const;

    /**
     * @brief Retrieves a color value from the theme.
     * @param id Identifier of the color.
     * @return The associated ThemeColor.
     * @throws std::runtime_error if not found.
     */
    [[nodiscard]] const Color& getColor(ColorId id) const;

    /**
     * @brief Retrieves a syntax highlight color by token type.
     * @param id The token identifier.
     * @return Reference to the color.
     * @throws std::runtime_error if not found.
     */
    [[nodiscard]] const Color& getColor(TokenId id) const;

    /**
     * @brief Returns glyph metadata for the given character.
     * @param character The UTF-16 character.
     * @return Reference to the glyph's atlas entry.
     * @throws std::runtime_error if not found.
     */
    [[nodiscard]] const AtlasEntry& getCharacter(char16_t character);

    /**
     * @brief Retrieves a dimension value by its identifier.
     * @param id The dimension identifier.
     * @return Dimension value in pixels.
     * @throws std::runtime_error if not found.
     */
    [[nodiscard]] int32_t getDimension(DimensionId id) const;

    /** @brief Returns the height of a text line in pixels. */
    [[nodiscard]] int32_t getLineHeight() const;

    /** @brief Returns the horizontal advance (spacing) between glyphs. */
    [[nodiscard]] int32_t getFontAdvance() const;

    /** @brief Returns the font descender (used for baseline alignment). */
    [[nodiscard]] int32_t getFontDescender() const;

    /**
     * @brief Calculates the rendered width of a UTF-16 string.
     * @param text Text to measure.
     * @param ignoreTabs If true, tabs are ignored in the measurement.
     * @return Width in pixels.
     */
    [[nodiscard]] int32_t measure(std::u16string_view text, bool ignoreTabs);
};


#endif //THEME_H
