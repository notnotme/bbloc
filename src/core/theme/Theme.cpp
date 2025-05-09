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

void Theme::create(CommandManager &commandManager, const std::string_view path) {
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
    registerThemeColorCVar(commandManager);
    registerHighLightColorCVar(commandManager);
    registerThemeDimensionCVar(commandManager);
}

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
    FT_Size_RequestRec font_size_req = {FT_SIZE_REQUEST_TYPE_CELL , 0, size << 6, 0, 0};
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

void Theme::registerThemeColorCVar(CommandManager &commandManager) {
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
    commandManager.registerCvar("col_margin_background",        cvar_margin_background_color.first->second);
    commandManager.registerCvar("col_info_bar_background",      cvar_info_bar_background_color.first->second);
    commandManager.registerCvar("col_editor_background",        cvar_editor_background_color.first->second);
    commandManager.registerCvar("col_prompt_background",        cvar_prompt_background_color.first->second);
    commandManager.registerCvar("col_current_line_background",  cvar_current_line_background_color.first->second);
    commandManager.registerCvar("col_selected_text_background", cvar_selected_text_background_color.first->second);
    commandManager.registerCvar("col_line_number",              cvar_line_number_color.first->second);
    commandManager.registerCvar("col_info_bar_text",            cvar_info_bar_text_color.first->second);
    commandManager.registerCvar("col_prompt_text",              cvar_prompt_text_color.first->second);
    commandManager.registerCvar("col_prompt_input_text",        cvar_prompt_input_text_color.first->second);
    commandManager.registerCvar("col_border",                   cvar_border_color.first->second);
    commandManager.registerCvar("col_cursor_indicator",         cvar_cursor_indicator_color.first->second);
}

void Theme::registerHighLightColorCVar(CommandManager &commandManager) {
    // Create default highlight colors
    const auto &cvar_hl_text_color           = m_highlight_colors.insert({TokenId::None,         std::make_shared<CVarColor>( 64,  64,  64, 255)});
    const auto &cvar_hl_comment_color        = m_highlight_colors.insert({TokenId::Comment,      std::make_shared<CVarColor>(160, 160, 160, 200)});
    const auto &cvar_hl_string_color         = m_highlight_colors.insert({TokenId::String,       std::make_shared<CVarColor>(  0, 150,   0, 255)});
    const auto &cvar_hl_preprocessor_color   = m_highlight_colors.insert({TokenId::Preprocessor, std::make_shared<CVarColor>(150, 150,  64, 255)});
    const auto &cvar_hl_number_color         = m_highlight_colors.insert({TokenId::Number,       std::make_shared<CVarColor>(  0, 200, 200, 255)});
    const auto &cvar_hl_keyword_color        = m_highlight_colors.insert({TokenId::Keyword,      std::make_shared<CVarColor>(  0,   0, 200, 255)});
    const auto &cvar_hl_statement_color      = m_highlight_colors.insert({TokenId::Statement,    std::make_shared<CVarColor>(200,   0, 200, 255)});

    // Make highlight colors accessible from the console
    commandManager.registerCvar("hl_text",          cvar_hl_text_color.first->second);
    commandManager.registerCvar("hl_comment",       cvar_hl_comment_color.first->second);
    commandManager.registerCvar("hl_string",        cvar_hl_string_color.first->second);
    commandManager.registerCvar("hl_preprocessor",  cvar_hl_preprocessor_color.first->second);
    commandManager.registerCvar("hl_number",        cvar_hl_number_color.first->second);
    commandManager.registerCvar("hl_keyword",       cvar_hl_keyword_color.first->second);
    commandManager.registerCvar("hl_statement",     cvar_hl_statement_color.first->second);
}

void Theme::registerThemeDimensionCVar(CommandManager &commandManager) {
    // Create default dimensions for the theme
    const auto &cvar_padding_width   = m_dimensions.insert({DimensionId::PaddingWidth,   std::make_shared<CVarInt>( 8)});
    const auto &cvar_indicator_width = m_dimensions.insert({DimensionId::IndicatorWidth, std::make_shared<CVarInt>( 2)});
    const auto &cvar_border_size     = m_dimensions.insert({DimensionId::BorderSize,     std::make_shared<CVarInt>( 1)});
    const auto &cvar_tab_to_space    = m_dimensions.insert({DimensionId::TabToSpace,     std::make_shared<CVarInt>( 4)});
    const auto &cvar_page_up_down    = m_dimensions.insert({DimensionId::PageUpDown,     std::make_shared<CVarInt>(10)});

    // Make dimensions accessible from the console
    commandManager.registerCvar("dim_padding_width",    cvar_padding_width.first->second);
    commandManager.registerCvar("dim_indicator_width",  cvar_indicator_width.first->second);
    commandManager.registerCvar("dim_border_size",      cvar_border_size.first->second);
    commandManager.registerCvar("dim_tab_to_space",     cvar_tab_to_space.first->second);
    commandManager.registerCvar("dim_page_up_down",     cvar_page_up_down.first->second);

    // Register a cvar to change the font size. It needs a callback.
    commandManager.registerCvar("dim_font_size", m_font_size, [&]{ setFontSize(m_font_size->m_value); });
}
