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
#include "InfoBar.h"

#include <algorithm>
#include <format>

#include <utf8.h>

#include "../ApplicationWindow.h"
#include "../core/theme/ColorId.h"
#include "../core/theme/DimensionId.h"


InfoBar::InfoBar(GlobalRegistry<CursorContext> &commandController, Theme &theme, QuadProgram &quadProgram)
    : View(commandController, theme, quadProgram) {}

void InfoBar::render(CursorContext &context, ViewState &viewState, QuadBuffer &quadBuffer, const float dt) {
    const auto batch_start = quadBuffer.beginBatch(ApplicationWindow::INFO_BAR_DEFAULT_QUAD_COUNT);
    drawBackground(quadBuffer, viewState);
    drawText(quadBuffer, context, viewState);
    const auto batch_count = quadBuffer.endBatch();

    // Get the vew geometry
    const auto position_x = viewState.getPositionX();
    const auto position_y = viewState.getPositionY();
    const auto width = viewState.getWidth();
    const auto height = viewState.getHeight();

    // Set the scissor area and draw the buffer
    glScissor(position_x, m_window_height - position_y - height, width, height);
    m_quad_program.draw(batch_start, batch_count);
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

void InfoBar::drawBackground(QuadBuffer &quadBuffer, const ViewState &viewState) const {
    // Get the vew geometry
    const auto position_x = viewState.getPositionX();
    const auto position_y = viewState.getPositionY();
    const auto width = viewState.getWidth();
    const auto height = viewState.getHeight();

    // Need some variables
    const auto &border_color = m_theme.getColor(ColorId::Border);
    const auto &background_color = m_theme.getColor(ColorId::InfoBarBackground);
    const auto border_size = m_theme.getDimension(DimensionId::BorderSize);

    drawQuad(quadBuffer, position_x, position_y, width, height - border_size, background_color);
    drawQuad(quadBuffer, position_x, position_y + height - border_size, width, border_size, border_color);
}

void InfoBar::drawText(QuadBuffer &quadBuffer, const CursorContext &context, const ViewState &viewState) const {
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
    auto string_cursor_name = utf8::utf8to16(context.cursor.getName().empty() ? "Untitled" : context.cursor.getName());
    if (context.cursor.isModified()) {
        // Mark the buffer as holding unsaved changes
        string_cursor_name.append(u"*");
    }
    if (context.buffer_count > 1) {
        // Several buffers are open: show the position of this one among them
        string_cursor_name.append(utf8::utf8to16(std::format(" [{}/{}]", context.buffer_index, context.buffer_count)));
    }
    const auto string_info = utf8::utf8to16(std::format("{} • {} • {}:{} / {}", font_size, highlight_mode, cursor_line + 1, cursor_column + 1, cursor_line_count));
    const auto string_info_size = m_theme.measure(string_info, true);
    const auto left_text_offset = static_cast<int16_t>(padding_width);
    const auto right_text_offset = static_cast<int16_t>(width - string_info_size - padding_width);
    const auto cursor_name_max_width = std::max(right_text_offset - left_text_offset - padding_width, 0);

    const auto strings = {
        std::pair { left_text_offset, m_theme.ellipsizeStart(string_cursor_name, cursor_name_max_width) },
        std::pair { right_text_offset, string_info }
    };

    const auto pen_position_y = position_y + line_height + font_descender;
    for (const auto &[x_offset, string] : strings) {
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
                    drawCharacter(quadBuffer, pen_position_x, pen_position_y, character, text_color);
                    pen_position_x += font_advance;
                break;
            }
        }
    }
}
