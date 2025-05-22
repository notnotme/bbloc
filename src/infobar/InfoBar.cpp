#include "InfoBar.h"

#include <format>
#include <utf8.h>

#include "../ApplicationWindow.h"
#include "../core/theme/ColorId.h"
#include "../core/theme/DimensionId.h"


InfoBar::InfoBar(CommandManager &commandManager, Theme &theme, QuadProgram &quadProgram, QuadBuffer &quadBuffer)
    : View(commandManager, theme, quadProgram, quadBuffer) {}

void InfoBar::render(CursorContext &context, ViewState &viewState, const float dt) {
    m_quad_buffer.map(ApplicationWindow::INFO_BAR_BUFFER_QUAD_OFFSET, ApplicationWindow::INFO_BAR_BUFFER_QUAD_COUNT);
    drawBackground(viewState);
    drawText(context, viewState);
    m_quad_buffer.unmap();

    // Get the vew geometry
    const auto position_x = viewState.getPositionX();
    const auto position_y = viewState.getPositionY();
    const auto width = viewState.getWidth();
    const auto height = viewState.getHeight();

    // Set the scissor area and draw the buffer
    glScissor(position_x, m_window_height - position_y - height, width, height);
    m_quad_program.draw(ApplicationWindow::INFO_BAR_BUFFER_QUAD_OFFSET, m_quad_buffer.getCount());
}

bool InfoBar::onKeyDown(CursorContext &context, ViewState &viewState, const SDL_Keycode keyCode, const uint16_t keyModifier) const {
    // No-op
    (void) context;
    (void) viewState;
    (void) keyCode;
    (void) keyModifier;
    return false;
}

void InfoBar::onTextInput(CursorContext &context, ViewState &viewState, const char *text) const {
    // No-op
    (void) context;
    (void) viewState;
    (void) text;
}

void InfoBar::drawBackground(const ViewState &viewState) const {
    // Get the vew geometry
    const auto position_x = viewState.getPositionX();
    const auto position_y = viewState.getPositionY();
    const auto width = viewState.getWidth();
    const auto height = viewState.getHeight();

    // Need some variables
    const auto &border_color = m_theme.getColor(ColorId::Border);
    const auto &background_color = m_theme.getColor(ColorId::InfoBarBackground);
    const auto border_size = m_theme.getDimension(DimensionId::BorderSize);

    drawQuad(position_x, position_y, width, height - border_size, background_color);
    drawQuad(position_x, position_y + height - border_size, width, border_size, border_color);
}

void InfoBar::drawText(const CursorContext &context, const ViewState &viewState) const {
    // Get the vew geometry
    const auto position_x = viewState.getPositionX();
    const auto position_y = viewState.getPositionY();
    const auto width = viewState.getWidth();

    // Need some variables
    const auto &text_color = m_theme.getColor(ColorId::InfoBarText);
    const auto line_height = m_theme.getLineHeight();
    const auto font_size = m_theme.getFontSize();
    const auto font_descender = m_theme.getFontDescender();
    const auto font_advance = m_theme.getFontAdvance();
    const auto tab_to_space = m_theme.getDimension(DimensionId::TabToSpace);
    const auto padding_width = m_theme.getDimension(DimensionId::PaddingWidth);
    const auto cursor_line = context.cursor.getLine();
    const auto cursor_column = context.cursor.getColumn();
    const auto cursor_line_count = context.cursor.getLineCount();
    const auto highlight_mode = context.highlighter.getModeString();

    // Build the informative strings, and calculate their x offset
    const auto string_cursor_name = utf8::utf8to16(context.cursor.getName().empty() ? "Untitled" : context.cursor.getName());
    const auto string_info = utf8::utf8to16(std::format("{} • {} • {}:{} / {}", font_size, highlight_mode, cursor_line + 1, cursor_column + 1, cursor_line_count));
    const auto string_info_size = m_theme.measure(string_info, true);
    const auto left_text_offset = static_cast<int16_t>(padding_width);
    const auto right_text_offset = static_cast<int16_t>(width - string_info_size - padding_width);

    const auto strings = {
        std::pair { left_text_offset, string_cursor_name },
        std::pair { right_text_offset, string_info }
    };

    auto quad_in_buffer = m_quad_buffer.getCount();
    const auto pen_position_y = position_y + line_height + font_descender;
    for (const auto &[x_offset, string] : strings) {
        if (quad_in_buffer == ApplicationWindow::INFO_BAR_BUFFER_QUAD_COUNT) {
            throw std::runtime_error("Not enough quad allowed to render the prompt.");
        }

        auto pen_position_x = position_x + x_offset;
        for (const auto c : string) {
            switch (c) {
                case ' ' :
                    pen_position_x += font_advance;
                break;
                case '\t' :
                    pen_position_x += font_advance * tab_to_space;
                break;
                default:
                    const auto &character = m_theme.getCharacter(c);
                    drawCharacter(pen_position_x, pen_position_y, character, text_color);
                    pen_position_x += font_advance;
                    ++quad_in_buffer;
                break;
            }

            // todo: fixme there is no reason to have this here, cursor name will be truncated if too long
            if (pen_position_x > position_x + width) {
                break;
            }
        }
    }
}
