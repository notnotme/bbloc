#include "InfoBar.h"

#include <format>
#include <utf8.h>

#include "../core/theme/ColorId.h"
#include "../core/theme/DimensionId.h"


InfoBar::InfoBar(CommandManager& commandManager, Theme& theme, QuadProgram& quadProgram, QuadBuffer& quadBuffer)
    : View(commandManager, theme, quadProgram, quadBuffer) {}

void InfoBar::create() {
}

void InfoBar::destroy() {
}

void InfoBar::render(const HighLighter& highLighter, const Cursor& cursor, InfoBarState& viewState, const float dt) {
    const auto position_x = viewState.getPositionX();
    const auto position_y = viewState.getPositionY();
    const auto width = viewState.getWidth();
    const auto height = viewState.getHeight();

    const auto& border_color = m_theme.getColor(ColorId::Border);
    const auto& text_color = m_theme.getColor(ColorId::InfoBarText);
    const auto& background_color = m_theme.getColor(ColorId::InfoBarBackground);
    const auto border_size = m_theme.getDimension(DimensionId::BorderSize);
    const auto tab_to_space = m_theme.getDimension(DimensionId::TabToSpace);
    const auto padding_width = m_theme.getDimension(DimensionId::PaddingWidth);

    const auto line_height = m_theme.getLineHeight();
    const auto font_size = m_theme.getFontSize();
    const auto font_descender = m_theme.getFontDescender();
    const auto font_advance = m_theme.getFontAdvance();

    const auto cursor_line = cursor.getLine();
    const auto cursor_column = cursor.getColumn();
    const auto cursor_line_count = cursor.getLineCount();

    /* todo: remove or move into a cvar ?
    const auto scroll_amount = cursor_line == 0
        ? cursor_line_count == 1 ? 100.0f : 0.0f
        : static_cast<float>(cursor_line) / static_cast<float>(cursor_line_count - 1) * 100.0f;
    const auto string_scroll_amount = std::format("{:.1f}%", scroll_amount);
    */

    const auto string_cursor_name = utf8::utf8to16(cursor.getName().empty() ? "Untitled" : cursor.getName());
    const auto string_info = utf8::utf8to16(std::format("{} • {} • {}:{} / {}", font_size, highLighter.getModeString(), cursor_line + 1, cursor_column + 1, cursor_line_count));
    const auto string_info_size = m_theme.measure(string_info, true);
    const auto strings = {
        std::pair {
            padding_width,
            string_cursor_name
        },
        std::pair {
            width - string_info_size - padding_width,
            string_info
        }
    };

    m_quad_buffer.map(0, 1024);
    // background
    m_quad_buffer.insert(
        position_x,
        position_y,
        width,
        height - border_size,
        background_color.red, background_color.green, background_color.blue, background_color.alpha);

    // bottom border
    m_quad_buffer.insert(
        static_cast<int16_t>(position_x),
        static_cast<int16_t>(position_y + line_height),
        width,
        border_size,
        border_color.red, border_color.green, border_color.blue, border_color.alpha);

    // text
    const auto pen_position_y = position_y + line_height + font_descender;
    for (const auto& string_item : strings) {
        auto pen_position_x = position_x + string_item.first;
        for (const auto c : string_item.second) {
            switch (c) {
                case ' ' :
                    pen_position_x += font_advance;
                break;
                case '\t' :
                    pen_position_x += font_advance * tab_to_space;
                break;
                default:
                    const auto& character = m_theme.getCharacter(c);
                    m_quad_buffer.insert(
                        static_cast<int16_t>(pen_position_x + character.bearing_x),
                        static_cast<int16_t>(pen_position_y - character.bearing_y),
                        character.width, character.height,
                        character.texture_s, character.texture_t, character.texture_p, character.texture_q, character.layer,
                        text_color.red, text_color.green, text_color.blue, text_color.alpha);

                        pen_position_x += font_advance;
                break;
            }

            // todo: fixme there is no reason to have this here, cursor name will be truncated if too long
            if (pen_position_x > position_x + width) {
                break;
            }
        }
    }
    m_quad_buffer.unmap();

    // Draw
    glScissor(
        position_x,
        m_window_height - position_y - height,
        width,
        height);

    m_quad_program.draw(0, m_quad_buffer.getCount());
}

bool InfoBar::onKeyDown(const HighLighter &highLighter, Cursor &cursor, InfoBarState &viewState, SDL_Keycode keyCode, uint16_t keyModifier) const {
    return false;
}

void InfoBar::onTextInput(const HighLighter &highLighter, Cursor &cursor, InfoBarState &viewState, const char *text) const {
}
