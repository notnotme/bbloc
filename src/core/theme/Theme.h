#ifndef THEME_H
#define THEME_H

#include <memory>
#include <unordered_map>
#include <string>
#include <string_view>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "../base/GlobalRegistry.h"
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
 * CommandController to expose runtime-modifiable theme variables.
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
    template <typename TPayload>
    void registerThemeColorCVar(GlobalRegistry<TPayload> &commandController);

    /** @brief Registers all syntax highlight color CVars. */
    template <typename TPayload>
    void registerHighLightColorCVar(GlobalRegistry<TPayload> &commandController);

    /** @brief Registers dimension CVars used for layout and spacing. */
    template <typename TPayload>
    void registerThemeDimensionCVar(GlobalRegistry<TPayload> &commandController);

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
     * @param commandController The CommandController to register CVars with.
     * @param path Filesystem path to the theme folder (must contain FONT_FILE).
     */
    template <typename TPayload>
    void create(GlobalRegistry<TPayload> &commandController, std::string_view path);

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
     * @brief Calculates the rendered width of a UTF-16 string.
     *
     * @param text Text to measure.
     * @param ignoreTabs If true, tabs are ignored in the measurement.
     * @return Width in pixels.
     */
    [[nodiscard]] int32_t measure(std::u16string_view text, bool ignoreTabs);
};

template<typename TPayload>
void Theme::create(GlobalRegistry<TPayload> &commandController, const std::string_view path) {
    // Create the atlas and texture
    m_atlas_array.create();
    m_quad_texture.create(0);

    // Set up the FT library and load theme text font
    FT_Init_FreeType(&m_ft_library);
    const auto font_file_path = std::string(path).append(FONT_FILE);
    if (FT_New_Face(m_ft_library, font_file_path.data(), 0, &m_font) != 0) {
        throw std::runtime_error(std::string("Theme::create: FT_New_Face failed: ").append(FONT_FILE));
    }

    if (!FT_IS_FIXED_WIDTH(m_font)) {
        // We need a fixed width font
        throw std::runtime_error("Theme::create: Font is not fixed width.");
    }

    setFontSize(DEFAULT_FONT_SIZE);
    registerThemeColorCVar(commandController);
    registerHighLightColorCVar(commandController);
    registerThemeDimensionCVar(commandController);
}

template<typename TPayload>
void Theme::registerThemeColorCVar(GlobalRegistry<TPayload> &commandController) {
    // Create default colors for the theme
    const auto &cvar_margin_background_color         = m_colors.insert({ColorId::MarginBackground,       std::make_shared<CVarColor>(220, 220, 220, 255)});
    const auto &cvar_info_bar_background_color       = m_colors.insert({ColorId::InfoBarBackground,      std::make_shared<CVarColor>(210, 210, 210, 255)});
    const auto &cvar_editor_background_color         = m_colors.insert({ColorId::EditorBackground,       std::make_shared<CVarColor>(250, 250, 250, 255)});
    const auto &cvar_prompt_background_color         = m_colors.insert({ColorId::PromptBackground,       std::make_shared<CVarColor>(210, 210, 210, 255)});
    const auto &cvar_current_line_background_color   = m_colors.insert({ColorId::LineBackground,         std::make_shared<CVarColor>(  0,   0,   0,  12)});
    const auto &cvar_selected_text_background_color  = m_colors.insert({ColorId::SelectedTextBackground, std::make_shared<CVarColor>(  0, 200, 255,  32)});
    const auto &cvar_line_number_color               = m_colors.insert({ColorId::LineNumber,             std::make_shared<CVarColor>(  0,   0,   0, 220)});
    const auto &cvar_info_bar_text_color             = m_colors.insert({ColorId::InfoBarText,            std::make_shared<CVarColor>(  0,   0,   0, 220)});
    const auto &cvar_prompt_text_color               = m_colors.insert({ColorId::PromptText,             std::make_shared<CVarColor>(  0,   0,   0, 220)});
    const auto &cvar_prompt_input_text_color         = m_colors.insert({ColorId::PromptInputText,        std::make_shared<CVarColor>(  0,   0,   0, 220)});
    const auto &cvar_border_color                    = m_colors.insert({ColorId::Border,                 std::make_shared<CVarColor>(150, 150, 150, 255)});
    const auto &cvar_cursor_indicator_color          = m_colors.insert({ColorId::CursorIndicator,        std::make_shared<CVarColor>(  0,   0,   0, 255)});

    // Make colors accessible from the console
    commandController.registerCvar(u"col_margin_background",        cvar_margin_background_color.first->second, nullptr);
    commandController.registerCvar(u"col_info_bar_background",      cvar_info_bar_background_color.first->second, nullptr);
    commandController.registerCvar(u"col_editor_background",        cvar_editor_background_color.first->second, nullptr);
    commandController.registerCvar(u"col_prompt_background",        cvar_prompt_background_color.first->second, nullptr);
    commandController.registerCvar(u"col_current_line_background",  cvar_current_line_background_color.first->second, nullptr);
    commandController.registerCvar(u"col_selected_text_background", cvar_selected_text_background_color.first->second, nullptr);
    commandController.registerCvar(u"col_line_number",              cvar_line_number_color.first->second, nullptr);
    commandController.registerCvar(u"col_info_bar_text",            cvar_info_bar_text_color.first->second, nullptr);
    commandController.registerCvar(u"col_prompt_text",              cvar_prompt_text_color.first->second, nullptr);
    commandController.registerCvar(u"col_prompt_input_text",        cvar_prompt_input_text_color.first->second, nullptr);
    commandController.registerCvar(u"col_border",                   cvar_border_color.first->second, nullptr);
    commandController.registerCvar(u"col_cursor_indicator",         cvar_cursor_indicator_color.first->second, nullptr);
}

template<typename TPayload>
void Theme::registerHighLightColorCVar(GlobalRegistry<TPayload> &commandController) {
    // Create default highlight colors
    const auto &cvar_hl_text_color           = m_highlight_colors.insert({TokenId::None,         std::make_shared<CVarColor>( 64,  64,  64, 255)});
    const auto &cvar_hl_comment_color        = m_highlight_colors.insert({TokenId::Comment,      std::make_shared<CVarColor>(160, 160, 160, 200)});
    const auto &cvar_hl_string_color         = m_highlight_colors.insert({TokenId::String,       std::make_shared<CVarColor>(  0, 150,   0, 255)});
    const auto &cvar_hl_preprocessor_color   = m_highlight_colors.insert({TokenId::Preprocessor, std::make_shared<CVarColor>(150, 150,  64, 255)});
    const auto &cvar_hl_number_color         = m_highlight_colors.insert({TokenId::Number,       std::make_shared<CVarColor>(  0, 200, 200, 255)});
    const auto &cvar_hl_keyword_color        = m_highlight_colors.insert({TokenId::Keyword,      std::make_shared<CVarColor>(  0,   0, 200, 255)});
    const auto &cvar_hl_statement_color      = m_highlight_colors.insert({TokenId::Statement,    std::make_shared<CVarColor>(200,   0, 200, 255)});

    // Make highlight colors accessible from the console
    commandController.registerCvar(u"hl_text",          cvar_hl_text_color.first->second, nullptr);
    commandController.registerCvar(u"hl_comment",       cvar_hl_comment_color.first->second, nullptr);
    commandController.registerCvar(u"hl_string",        cvar_hl_string_color.first->second, nullptr);
    commandController.registerCvar(u"hl_preprocessor",  cvar_hl_preprocessor_color.first->second, nullptr);
    commandController.registerCvar(u"hl_number",        cvar_hl_number_color.first->second, nullptr);
    commandController.registerCvar(u"hl_keyword",       cvar_hl_keyword_color.first->second, nullptr);
    commandController.registerCvar(u"hl_statement",     cvar_hl_statement_color.first->second, nullptr);
}

template<typename TPayload>
void Theme::registerThemeDimensionCVar(GlobalRegistry<TPayload> &commandController) {
    // Create default dimensions for the theme
    const auto &cvar_padding_width   = m_dimensions.insert({DimensionId::PaddingWidth,   std::make_shared<CVarInt>( 8)});
    const auto &cvar_indicator_width = m_dimensions.insert({DimensionId::IndicatorWidth, std::make_shared<CVarInt>( 2)});
    const auto &cvar_border_size     = m_dimensions.insert({DimensionId::BorderSize,     std::make_shared<CVarInt>( 1)});
    const auto &cvar_tab_to_space    = m_dimensions.insert({DimensionId::TabToSpace,     std::make_shared<CVarInt>( 4)});
    const auto &cvar_page_up_down    = m_dimensions.insert({DimensionId::PageUpDown,     std::make_shared<CVarInt>(10)});

    // Make dimensions accessible from the console
    commandController.registerCvar(u"dim_padding_width",    cvar_padding_width.first->second, nullptr);
    commandController.registerCvar(u"dim_indicator_width",  cvar_indicator_width.first->second, nullptr);
    commandController.registerCvar(u"dim_border_size",      cvar_border_size.first->second, nullptr);
    commandController.registerCvar(u"dim_tab_to_space",     cvar_tab_to_space.first->second, nullptr);
    commandController.registerCvar(u"dim_page_up_down",     cvar_page_up_down.first->second, nullptr);

    // Register a cvar to change the font size. It needs a callback.
    commandController.registerCvar(u"dim_font_size", m_font_size, [&]{ setFontSize(m_font_size->m_value); });
}


#endif //THEME_H
