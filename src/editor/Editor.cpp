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
#include "Editor.h"

#include <iostream>
#include <algorithm>
#include <array>
#include <limits>
#include <ranges>
#include <utf8.h>

#include "../core/theme/DimensionId.h"
#include "../core/theme/TabStop.h"


/**
 * @brief Projects a content-space coordinate onto the viewport, saturating what int32 cannot hold.
 *
 * This is the single narrowing between the 64-bit content/scroll space and the 32-bit screen
 * space: a quad pushed past the range saturates at the edge (where drawQuad clips it anyway)
 * instead of wrapping around to the opposite side.
 */
static int32_t projectToViewport(const int64_t value) {
    constexpr auto min_value = static_cast<int64_t>(std::numeric_limits<int32_t>::min());
    constexpr auto max_value = static_cast<int64_t>(std::numeric_limits<int32_t>::max());
    return static_cast<int32_t>(std::clamp(value, min_value, max_value));
}

Editor::Editor(GlobalRegistry<CursorContext> &commandController, Theme &theme, QuadProgram &quadProgram)
    : View(commandController, theme, quadProgram),
      m_is_tab_to_space(std::make_shared<CVarBool>(true)),
      m_show_scrollbar(std::make_shared<CVarBool>(true)),
      m_mouse_drag(MouseDrag::None),
      m_drag_grab(0),
      m_drag_scroll(0),
      m_drag_line(0),
      m_drag_column(0) {
    // Register cvars
    registerTabToSpaceCVar();
    registerShowScrollbarCVar();
}

void Editor::render(CursorContext &context, ViewState &viewState, QuadBuffer &quadBuffer, const float dt) {
    (void) dt;
    // The margin width, the longest line and the scrollbar sizes are invariant for the whole
    // frame, and measuring the longest line rescans every line metric when it is dirty: resolve
    // everything once and pass it down. The mouse handlers resolve the same metrics.
    const auto metrics = computeFrameMetrics(context, viewState);
    const auto border_size = m_theme.getDimension(DimensionId::BorderSize);
    const auto margin_width = metrics.margin_width;
    const auto v_bar_width = metrics.v_bar_width;
    const auto h_bar_height = metrics.h_bar_height;
    const auto longest_line_length = metrics.longest_line_length;

    // Follow or scroll to the said position. scrollX and scrollY and followIndicator are eventually updated outside (by reference)
    updateScroll(context, viewState, margin_width, v_bar_width, h_bar_height, longest_line_length);

    // Get updated scroll values
    const auto scroll_x = context.scroll.x;
    const auto scroll_y = context.scroll.y;

    // Begin the editor batch, keep a variable to know how many quads we have before the cursor text
    const auto batch_start = quadBuffer.beginBatch(DEFAULT_QUAD_COUNT);
    drawBackground(quadBuffer, viewState, margin_width);
    drawMarginText(quadBuffer, context, viewState, metrics.line_count_width, scroll_y);
    drawScrollbars(quadBuffer, context, viewState, margin_width, v_bar_width, h_bar_height, longest_line_length);

    // Read mid-batch on purpose: this is the split point between the two draws below.
    const auto quads_count_before_text = quadBuffer.getCount();
    drawText(quadBuffer, context, viewState, scroll_x, scroll_y, margin_width);
    const auto batch_count = quadBuffer.endBatch();

    // Get the vew geometry
    const auto position_x = viewState.getPositionX();
    const auto position_y = viewState.getPositionY();
    const auto width = viewState.getWidth();
    const auto height = viewState.getHeight();

    // Draw backgrounds and line number
    glScissor(position_x, m_window_height - position_y - height, width, height);
    m_quad_program.draw(batch_start, quads_count_before_text);

    // Draw cursor text, keeping glyphs from drawing under the scrollbars.
    // A collapsed text area (huge font, or a wide margin in a narrow window) would make these
    // negative. GL rejects a negative scissor size and keeps the previous box, which would let the
    // text paint over the margin and the scrollbars, so clamp and skip the draw when nothing fits.
    const auto text_scissor_width = std::max(0, width - margin_width - border_size - v_bar_width);
    const auto text_scissor_height = std::max(0, height - h_bar_height);
    if (text_scissor_width > 0 && text_scissor_height > 0) {
        glScissor(position_x + margin_width + border_size, m_window_height - position_y - height + h_bar_height, text_scissor_width, text_scissor_height);

        const auto draw_offset = batch_start + quads_count_before_text;
        m_quad_program.draw(draw_offset, batch_count - quads_count_before_text);
    }
}

bool Editor::onKeyDown(CursorContext &context, ViewState &viewState, const SDL_Keycode keyCode, const uint16_t keyModifier) const {
    (void) viewState;
    (void) keyModifier;
    switch (keyCode) {
        case SDLK_RETURN: {
            context.scroll.follow_indicator = true;
            // Any new inputs deactivate the selection and cut the previously selected text before inserting the new input
            context.eraseSelectionIfAny();

            const auto &edit = context.cursor.newLine();
            context.highlighter.edit(edit);
            // Update stick to column index if we insert
            context.stick.index = context.cursor.getColumn();
        }
        return true;
        case SDLK_BACKSPACE: {
            context.scroll.follow_indicator = true;
            // Any new inputs deactivate the selection and cut the previously selected text before inserting the new input
            if (!context.eraseSelectionIfAny()) {
                if (const auto &edit = context.cursor.eraseLeft()) {
                    context.highlighter.edit(edit.value());
                }
            }
            // Do not update stick to column index if we remove
        }
        return true;
        case SDLK_DELETE: {
            context.scroll.follow_indicator = true;
            // Any new inputs deactivate the selection and cut the previously selected text before inserting the new input
            if (!context.eraseSelectionIfAny()) {
                if (const auto &edit = context.cursor.eraseRight()) {
                    context.highlighter.edit(edit.value());
                }
            }
            // Update stick to column index
            context.stick.index = context.cursor.getColumn();
        }
        return true;
        case SDLK_TAB: {
            context.scroll.follow_indicator = true;
            // Any new inputs deactivate the selection and cut the previously selected text before inserting the new input
            context.eraseSelectionIfAny();

            if (m_is_tab_to_space->m_value) {
                // Pad up to the next tab stop rather than inserting a fixed run of spaces,
                // so the caret lands where the neighbouring lines align.
                const uint32_t tab_width = static_cast<uint32_t>(std::max(m_theme.getDimension(DimensionId::TabToSpace), 1));
                const uint32_t column = context.cursor.getColumn();

                // The visual column only deviates from the character index by the tabs
                // before the caret, each snapping to its own tab stop.
                uint32_t visual_column = column;
                if (const uint32_t line = context.cursor.getLine(); context.cursor.getLineTabCount(line) != 0) {
                    const std::u16string_view text = context.cursor.getString(line);
                    visual_column = visualColumns(text.substr(0, column), tab_width);
                }

                const uint32_t space_amount = tab_width - visual_column % tab_width;
                const auto &edit = context.cursor.insert(std::u16string(space_amount, u' '));
                context.highlighter.edit(edit);
            } else {
                const auto &edit = context.cursor.insert(u"\t");
                context.highlighter.edit(edit);
            }
            // Update stick to column index
            context.stick.index = context.cursor.getColumn();
        }
        return true;
        default:
        return false;
    }
}

void Editor::onTextInput(CursorContext &context, ViewState &viewState, const char *text) const {
    (void) viewState;
    auto utf8_text = std::string(text);
    if (!utf8::is_valid(utf8_text)) {
        // throw std::runtime_error("Invalid UTF-8 text: " + utf8_text);
        // Let's replace it by the diamond interrogation mark instead of throwing an exception
        std::cerr << "Invalid UTF-8 text: " << utf8_text << std::endl;

        utf8_text = utf8::replace_invalid(utf8_text);
    }

    // Any new inputs deactivate the selection and cut the previously selected text before inserting the new input
    context.eraseSelectionIfAny();

    const auto utf16_text = utf8::utf8to16(utf8_text);
    const auto &edit = context.cursor.insert(utf16_text);
    context.stick.index = context.cursor.getColumn();
    context.scroll.follow_indicator = true;
    context.highlighter.edit(edit);
}

void Editor::updateScroll(CursorContext &context, const ViewState &viewState, const int32_t marginWidth, const int32_t vBarWidth, const int32_t hBarHeight, const uint32_t longestLineLength) const {
    const auto line_height = m_theme.getLineHeight();
    const auto border_size = m_theme.getDimension(DimensionId::BorderSize);
    const auto indicator_width = m_theme.getDimension(DimensionId::IndicatorWidth);

    const auto cursor_line = context.cursor.getLine();
    const auto cursor_column = context.cursor.getColumn();
    const auto cursor_line_count = context.cursor.getLineCount();

    const auto width = viewState.getWidth();
    const auto height = viewState.getHeight();

    if (context.scroll.follow_indicator) {
        const auto scroll_x = context.scroll.x;
        const auto scroll_y = context.scroll.y;
        const auto cursor_string = context.cursor.getString();
        const auto cursor_string_to_indicator = cursor_string.substr(0, cursor_column);
        const auto indicator_x = measureLineText(context, cursor_line, cursor_string_to_indicator);
        const auto indicator_y = static_cast<int64_t>(line_height) * cursor_line;

        // Vertical scroll
        if (indicator_y < scroll_y) {
            context.scroll.y = indicator_y;
        } else if (indicator_y > height - hBarHeight + scroll_y - line_height) {
            context.scroll.y = indicator_y - (height - hBarHeight - line_height);
        }

        // Horizontal scroll
        if (indicator_x < scroll_x) {
            context.scroll.x = indicator_x;
        } else if (indicator_x > width - marginWidth - border_size - vBarWidth + scroll_x) {
            context.scroll.x = indicator_x - width + marginWidth + border_size + vBarWidth + indicator_width;
        }
    } else {
        // Update max-scroll values
        const auto scroll_x = context.scroll.x;
        const auto scroll_y = context.scroll.y;
        const auto longest_line_width = contentWidth(longestLineLength);
        const auto max_scroll_y = static_cast<int64_t>(cursor_line_count) * line_height - (height - hBarHeight);
        const auto max_scroll_x = longest_line_width - (width - marginWidth - border_size - indicator_width - vBarWidth);
        context.scroll.x = std::clamp(scroll_x, int64_t{0}, max_scroll_x < 0 ? int64_t{0} : max_scroll_x);
        context.scroll.y = std::clamp(scroll_y, int64_t{0}, max_scroll_y < 0 ? int64_t{0} : max_scroll_y);
    }
}

int64_t Editor::contentHeight(const CursorContext &context) const {
    return static_cast<int64_t>(context.cursor.getLineCount()) * m_theme.getLineHeight();
}

int64_t Editor::contentWidth(const uint32_t longestLineLength) const {
    return static_cast<int64_t>(longestLineLength) * m_theme.getFontAdvance();
}

int64_t Editor::measureLineText(const CursorContext &context, const uint32_t line, const std::u16string_view text) const {
    const auto font_advance = m_theme.getFontAdvance();
    const auto length = static_cast<int64_t>(text.length());

    // The visual width only deviates from length * advance on tabs, and the buffer tracks the
    // tab count of every line: a tab-free line needs no walk at all
    if (context.cursor.getLineTabCount(line) == 0) {
        return length * font_advance;
    }

    // Tabs snap to tab stops, so the measure is only valid for a prefix starting at visual
    // column 0 — every caller measures such a prefix. O(length) on tabby lines, but the scan
    // folds tab-free runs so it stays far cheaper than a per-character walk.
    const uint32_t tab_width = static_cast<uint32_t>(std::max(m_theme.getDimension(DimensionId::TabToSpace), 1));
    return static_cast<int64_t>(visualColumns(text, tab_width)) * font_advance;
}

void Editor::drawBackground(QuadBuffer &quadBuffer, const ViewState &viewState, const int32_t marginWidth) const {
    // Get the vew geometry
    const auto position_x = viewState.getPositionX();
    const auto position_y = viewState.getPositionY();
    const auto width = viewState.getWidth();
    const auto height = viewState.getHeight();

    // Need some variables
    const auto &border_color = m_theme.getColor(ColorId::Border);
    const auto &background_color = m_theme.getColor(ColorId::EditorBackground);
    const auto &margin_color = m_theme.getColor(ColorId::MarginBackground);
    const auto border_size = m_theme.getDimension(DimensionId::BorderSize);

    // Draw left background margin, right border and editor background -> 3 quads
    drawQuad(quadBuffer, position_x, position_y, marginWidth, height, margin_color);
    drawQuad(quadBuffer, position_x + marginWidth, position_y, border_size, height, border_color);
    drawQuad(quadBuffer, position_x + marginWidth + border_size, position_y, width - marginWidth - border_size, height, background_color);
}

void Editor::drawMarginText(QuadBuffer &quadBuffer, const CursorContext &context, const ViewState &viewState, const int32_t lineCountWidth, const int64_t scrollY) const {
    // Get the vew geometry
    const auto position_x = viewState.getPositionX();
    const auto position_y = viewState.getPositionY();
    const auto height = viewState.getHeight();

    // Need some variables
    const auto &line_number_color = m_theme.getColor(ColorId::LineNumber);
    const auto padding_width = m_theme.getDimension(DimensionId::PaddingWidth);
    const auto line_height = m_theme.getLineHeight();
    const auto font_descender = m_theme.getFontDescender();
    const auto font_advance = m_theme.getFontAdvance();
    const auto cursor_line_count = context.cursor.getLineCount();

    // Draw text. The scroll offset within the first line is bounded by the line height, so it is
    // the one place the 64-bit scroll re-enters the 32-bit screen space.
    const auto first_line_in_viewport = scrollY / line_height;
    const auto line_scroll_offset_y = static_cast<int32_t>(-scrollY % line_height);
    auto pen_position_y = line_scroll_offset_y + position_y + line_height + font_descender;
    auto line_index = first_line_in_viewport;

    // Line numbers are ASCII digits, format them into a stack buffer to avoid per-line allocations
    constexpr auto max_line_number_digits = std::numeric_limits<uint32_t>::digits10 + 1;
    std::array<char16_t, max_line_number_digits> line_number_digits{};

    while (line_index < cursor_line_count) {
        if (line_index >= 0) {
            // Fill the buffer from its end, least significant digit first
            auto digit_count = 0;
            for (auto remainder = static_cast<uint32_t>(line_index) + 1; remainder > 0; remainder /= 10) {
                line_number_digits[max_line_number_digits - 1 - digit_count] = static_cast<char16_t>(u'0' + remainder % 10);
                ++digit_count;
            }

            auto pen_position_x = position_x + padding_width + lineCountWidth - digit_count * font_advance;
            for (auto digit_index = max_line_number_digits - digit_count; digit_index < max_line_number_digits; ++digit_index) {
                const auto &character = m_theme.getCharacter(line_number_digits[digit_index]);
                drawCharacter(quadBuffer, pen_position_x, pen_position_y, character, line_number_color);
                pen_position_x += font_advance;
            }
        }

        pen_position_y += line_height;
        if (pen_position_y >= position_y + height + line_height + font_descender) {
            // There is no need to continue at this point, all remaining lines are hidden
            break;
        }

        ++line_index;
    }
}

void Editor::drawText(QuadBuffer &quadBuffer, const CursorContext &context, const ViewState &viewState, const int64_t scrollX, const int64_t scrollY, const int32_t marginWidth) const {
    // Get the vew geometry
    const auto position_x = viewState.getPositionX();
    const auto position_y = viewState.getPositionY();
    const auto width = viewState.getWidth();
    const auto height = viewState.getHeight();

    // Need some variable
    const auto indicator_width = m_theme.getDimension(DimensionId::IndicatorWidth);
    const auto border_size = m_theme.getDimension(DimensionId::BorderSize);
    const uint32_t tab_width = static_cast<uint32_t>(std::max(m_theme.getDimension(DimensionId::TabToSpace), 1));

    const auto line_height = m_theme.getLineHeight();
    const auto font_descender = m_theme.getFontDescender();
    const auto font_advance = m_theme.getFontAdvance();

    const auto cursor_line = context.cursor.getLine();
    const auto cursor_column = context.cursor.getColumn();
    const auto cursor_line_count = context.cursor.getLineCount();

    const auto cursor_text_start_x = position_x + marginWidth + border_size;

    // Draw text. The scroll offset within the first line is bounded by the line height, so it is
    // the one place the 64-bit vertical scroll re-enters the 32-bit screen space.
    const auto first_line_in_viewport = scrollY / line_height;
    const auto line_scroll_offset_y = static_cast<int32_t>(-scrollY % line_height);
    auto pen_position_y = line_scroll_offset_y + position_y + line_height + font_descender;
    auto line_index = first_line_in_viewport;

    while (line_index < cursor_line_count) {
        if (line_index >= 0) {
            // The guard above makes the narrowing exact: the index designates a valid buffer line
            const auto line = static_cast<uint32_t>(line_index);

            // Get the string at line, and its highlight row: the row is invariant for the
            // whole line, fetching it per glyph would redo every guard of the highlighter cache
            const auto string = context.cursor.getString(line);
            const auto high_light_line = context.highlighter.getHighLightLine(line);
            const auto string_length = static_cast<uint32_t>(string.length());
            const auto is_cursor_line = cursor_line == line;

            if (is_cursor_line) {
                // Begin current line bg
                const auto &line_background_color = m_theme.getColor(ColorId::LineBackground);
                drawQuad(quadBuffer, cursor_text_start_x, pen_position_y - line_height - font_descender, width, line_height, line_background_color);
            }

            if (const auto &selected_range = context.cursor.getSelectedRange()) {
                // Check if the selected range is in the viewport
                const auto &selected_background_color = m_theme.getColor(ColorId::SelectedTextBackground);
                if (selected_range->line_start == line && selected_range->line_end == line) {
                    // The selection start / end on the same line. Select only a range of text.
                    // Both bounds are measured as line prefixes: a mid-line slice has no tab-stop
                    // origin of its own, so the width is the difference of the two prefixes.
                    const auto selection_start_x = measureLineText(context, line, string.substr(0, selected_range->column_start));
                    const auto selected_text_width = measureLineText(context, line, string.substr(0, selected_range->column_end)) - selection_start_x;
                    drawQuad(quadBuffer, projectToViewport(cursor_text_start_x - scrollX + selection_start_x), pen_position_y - line_height - font_descender, projectToViewport(selected_text_width), line_height, selected_background_color);
                } else if (line == selected_range->line_start) {
                    // First line of selected text, the selection starts at column until the end of the text area.
                    const auto selection_start_x = measureLineText(context, line, string.substr(0, selected_range->column_start));
                    drawQuad(quadBuffer, projectToViewport(cursor_text_start_x - scrollX + selection_start_x), pen_position_y - line_height - font_descender, projectToViewport(width - selection_start_x), line_height, selected_background_color);
                } else if (line == selected_range->line_end) {
                    // Last line of selected text, the selection starts at the margin border, until the end column.
                    const auto selected_text = string.substr(0, selected_range->column_end);
                    const auto selected_text_width = measureLineText(context, line, selected_text);
                    drawQuad(quadBuffer, projectToViewport(cursor_text_start_x - scrollX), pen_position_y - line_height - font_descender, projectToViewport(selected_text_width), line_height, selected_background_color);
                } else if (line > selected_range->line_start && line < selected_range->line_end) {
                    // In between two selected lines, the selection takes the whole width
                    drawQuad(quadBuffer, cursor_text_start_x, pen_position_y - line_height - font_descender, width, line_height, selected_background_color);
                }
            }

            // Start drawing a new line, starting from the first character visible in the viewport, until the end of the string.
            // Columns scrolled out on the left cannot produce quads, and every character before the
            // first tab is exactly one font advance wide: jump the walk straight to the last column
            // that still ends left of the viewport (capped at the line length: past it the walk has
            // nothing to emit anyway), and at the first tab so tab widths keep being measured by
            // the walk itself. The emitted quads are identical to a walk from column 0.
            uint32_t start_column = 0;
            if (const auto scrolled_out_width = scrollX - marginWidth - border_size; scrolled_out_width > font_advance) {
                start_column = static_cast<uint32_t>(std::min<int64_t>(scrolled_out_width / font_advance - 1, string_length));
                if (context.cursor.getLineTabCount(line) > 0) {
                    // The line holds tabs: the search stops at the first one. Tab-free lines skip
                    // it entirely, keeping the whole jump independent of the line length.
                    if (const auto first_tab = string.substr(0, start_column).find(u'\t'); first_tab != std::u16string_view::npos) {
                        start_column = static_cast<uint32_t>(first_tab);
                    }
                }
            }

            // The skipped prefix is tab-free, so its width (and the cursor offset inside it) is a
            // plain multiply, and its character index doubles as its visual column
            auto cursor_position_x = projectToViewport(cursor_text_start_x - scrollX + static_cast<int64_t>(std::min(cursor_column, start_column)) * font_advance);
            auto pen_position_x = projectToViewport(cursor_text_start_x - scrollX + static_cast<int64_t>(start_column) * font_advance);
            uint32_t visual_column = start_column;

            for (auto character_column = start_column; character_column < string_length; ++character_column) {
                if (pen_position_x > position_x + width) {
                    // Nothing more is visible
                    break;
                }

                switch (const auto c = string[character_column]) {
                    case ' ':
                        pen_position_x += font_advance;
                        ++visual_column;
                    break;
                    case '\t': {
                        // A tab advances the pen to the next tab stop, 1 to tab_width columns away
                        const uint32_t next_tab_stop = nextTabStop(visual_column, tab_width);
                        pen_position_x += font_advance * static_cast<int32_t>(next_tab_stop - visual_column);
                        visual_column = next_tab_stop;
                    }
                    break;
                    default:
                        if (pen_position_x + font_advance >= position_x) {
                            // Only fetch characters and insert if it could be visible. The row can be
                            // shorter than the line, columns past its end are unpainted.
                            const auto token_id = character_column < high_light_line.size() ? high_light_line[character_column] : TokenId::None;
                            const auto &character = m_theme.getCharacter(c);
                            const auto &character_color = m_theme.getColor(token_id);
                            drawCharacter(quadBuffer, pen_position_x, pen_position_y, character, character_color);
                        }
                        pen_position_x += font_advance;
                        ++visual_column;
                    break;
                }

                if (is_cursor_line && character_column < cursor_column) {
                    cursor_position_x = pen_position_x;
                }
            }

            if (is_cursor_line) {
                // begin indicator
                const auto &indicator_color = m_theme.getColor(ColorId::CursorIndicator);
                drawQuad(quadBuffer, cursor_position_x, pen_position_y - line_height - font_descender, indicator_width, line_height, indicator_color);
            }
        }

        pen_position_y += line_height;
        if (pen_position_y >= position_y + height + line_height + font_descender) {
            // There is no need to continue at this point, all remaining lines are hidden
            break;
        }

        ++line_index;
    }
}

void Editor::computeScrollbarSizes(const CursorContext &context, const ViewState &viewState, const int32_t marginWidth, const uint32_t longestLineLength, int32_t &vBarWidth, int32_t &hBarHeight) const {
    vBarWidth = 0;
    hBarHeight = 0;
    if (!m_show_scrollbar->m_value) {
        return;
    }

    // Need some variables
    const auto bar_size = m_theme.getDimension(DimensionId::ScrollbarWidth);
    const auto border_size = m_theme.getDimension(DimensionId::BorderSize);

    const auto height = viewState.getHeight();
    const auto text_width = viewState.getWidth() - marginWidth - border_size;

    // Content extents live in 64 bits: a line count or line length near 4G overflows 32-bit pixels
    const auto content_height = contentHeight(context);
    const auto content_width = contentWidth(longestLineLength);

    // A bar thicker than what is left on the axis it reserves space on cannot be shown at all:
    // it would eat the whole text area and push what remains of it negative.
    const auto v_fits = bar_size <= text_width;
    const auto h_fits = bar_size <= height;

    // Each bar consumes space on the other axis, so a bar becoming visible can make the other one
    // necessary. Two rounds are enough: first decide with the full sizes, then with the reduced ones.
    // Only the horizontal first-round decision feeds the second round: the vertical one would be
    // recomputed before ever being read.
    const auto h_visible_first_round = h_fits && content_width > text_width;
    const auto v_visible = v_fits && content_height > height - (h_visible_first_round ? bar_size : 0);
    const auto h_visible = h_fits && content_width > text_width - (v_visible ? bar_size : 0);

    vBarWidth = v_visible ? bar_size : 0;
    hBarHeight = h_visible ? bar_size : 0;
}

void Editor::drawScrollbars(QuadBuffer &quadBuffer, const CursorContext &context, const ViewState &viewState, const int32_t marginWidth, const int32_t vBarWidth, const int32_t hBarHeight, const uint32_t longestLineLength) const {
    if (vBarWidth == 0 && hBarHeight == 0) {
        // The scrollbars are disabled, or the content fits entirely in the view and both bars are auto-hidden
        return;
    }

    // Need some variables
    const auto border_size = m_theme.getDimension(DimensionId::BorderSize);

    const auto &track_color = m_theme.getColor(ColorId::ScrollbarBackground);
    const auto &thumb_color = m_theme.getColor(ColorId::ScrollbarThumb);

    // Get the vew geometry
    const auto position_x = viewState.getPositionX();
    const auto position_y = viewState.getPositionY();
    const auto width = viewState.getWidth();
    const auto height = viewState.getHeight();

    if (vBarWidth > 0) {
        // The track is drawn full-height on purpose: it also covers the corner square when both bars are visible
        drawQuad(quadBuffer, position_x + width - vBarWidth, position_y, vBarWidth, height, track_color);

        // Thumb size and position are proportional to the visible / content heights (64-bit intermediates)
        const auto view_h = static_cast<int64_t>(height - hBarHeight);
        if (view_h > 0) {
            const auto bar = computeScrollbarMetrics(position_y, view_h, contentHeight(context), context.scroll.y);
            drawQuad(quadBuffer, position_x + width - vBarWidth, static_cast<int32_t>(bar.thumb_origin), vBarWidth, static_cast<int32_t>(bar.thumb_size), thumb_color);
        }
    }

    if (hBarHeight > 0) {
        // The horizontal bar spans the text area only, and stops before the vertical bar
        const auto text_x = position_x + marginWidth + border_size;
        const auto view_w = static_cast<int64_t>(width - marginWidth - border_size - vBarWidth);
        if (view_w > 0) {
            drawQuad(quadBuffer, text_x, position_y + height - hBarHeight, static_cast<int32_t>(view_w), hBarHeight, track_color);

            // Thumb size and position are proportional to the visible / content widths (64-bit intermediates)
            const auto bar = computeScrollbarMetrics(text_x, view_w, contentWidth(longestLineLength), context.scroll.x);
            drawQuad(quadBuffer, static_cast<int32_t>(bar.thumb_origin), position_y + height - hBarHeight, static_cast<int32_t>(bar.thumb_size), hBarHeight, thumb_color);
        }
    }
}

Editor::FrameMetrics Editor::computeFrameMetrics(const CursorContext &context, const ViewState &viewState) const {
    const auto padding_width = m_theme.getDimension(DimensionId::PaddingWidth);
    const auto cursor_line_count = context.cursor.getLineCount();

    // The greatest line number is as wide as the line count has digits
    auto line_count_digits = 0;
    for (auto remainder = cursor_line_count; remainder > 0 || line_count_digits == 0; remainder /= 10) {
        ++line_count_digits;
    }

    const auto line_count_width = line_count_digits * m_theme.getFontAdvance();
    const auto tab_to_space = static_cast<uint32_t>(m_theme.getDimension(DimensionId::TabToSpace));

    // The scrollbar sizes are filled in place right below, through the two out parameters.
    auto metrics = FrameMetrics{
        .margin_width = padding_width + line_count_width + padding_width,
        .line_count_width = line_count_width,
        .longest_line_length = context.cursor.getLongestLineLength(tab_to_space)
    };
    computeScrollbarSizes(context, viewState, metrics.margin_width, metrics.longest_line_length, metrics.v_bar_width, metrics.h_bar_height);
    return metrics;
}

Editor::ScrollbarMetrics Editor::computeScrollbarMetrics(const int64_t trackOrigin, const int64_t viewSize, const int64_t contentSize, const int64_t scroll) {
    const auto content_size = std::max(int64_t{1}, contentSize);
    const auto thumb_size = std::min(viewSize, std::max(MIN_THUMB_SIZE, viewSize * viewSize / content_size));
    const auto thumb_origin = trackOrigin + scroll * (viewSize - thumb_size) / std::max(int64_t{1}, content_size - viewSize);

    return ScrollbarMetrics{
        .view_size = viewSize,
        .content_size = content_size,
        .thumb_size = thumb_size,
        .thumb_origin = std::clamp(thumb_origin, trackOrigin, trackOrigin + viewSize - thumb_size)
    };
}

void Editor::applyThumbDrag(int64_t &scroll, const int32_t pointer, const ScrollbarMetrics &bar, CursorContext &context) const {
    // A thumb filling the whole track has no play and cannot move
    const auto track_play = bar.view_size - bar.thumb_size;
    if (track_play <= 0) {
        return;
    }

    // Map the pointer travel since the grab back to a scroll offset, the exact inverse of the
    // thumb positioning in computeScrollbarMetrics
    const auto max_scroll = std::max(int64_t{0}, bar.content_size - bar.view_size);
    const auto travel = static_cast<int64_t>(pointer - m_drag_grab);
    const auto target = std::clamp(m_drag_scroll + travel * max_scroll / track_play, int64_t{0}, max_scroll);
    if (target != scroll) {
        scroll = target;
        context.wants_redraw = true;
    }
}

uint32_t Editor::columnAtPixel(const CursorContext &context, const uint32_t line, const int64_t targetX) const {
    if (targetX <= 0) {
        // In or left of the margin: clamp to the beginning of the line
        return 0;
    }

    const auto string = context.cursor.getString(line);
    const auto string_length = static_cast<uint32_t>(string.length());
    const auto font_advance = m_theme.getFontAdvance();
    const uint32_t tab_width = static_cast<uint32_t>(std::max(m_theme.getDimension(DimensionId::TabToSpace), 1));

    // Same fast-skip as drawText: every character before the first tab is exactly one font
    // advance wide, so the walk can start right at the column holding the pixel, capped at the
    // first tab so tab widths keep being measured by the walk itself.
    auto start_column = static_cast<uint32_t>(std::min<int64_t>(targetX / font_advance, string_length));
    if (context.cursor.getLineTabCount(line) > 0) {
        if (const auto first_tab = string.substr(0, start_column).find(u'\t'); first_tab != std::u16string_view::npos) {
            start_column = static_cast<uint32_t>(first_tab);
        }
    }

    // Walk the remaining columns with the render advances and stop at the first boundary whose
    // character midpoint lies right of the pixel: a click on the right half of a character
    // places the caret after it. The pen walks in content space, like the pixel it resolves.
    // The skipped prefix is tab-free, so its character index doubles as its visual column.
    auto pen_position_x = static_cast<int64_t>(start_column) * font_advance;
    uint32_t visual_column = start_column;
    for (auto character_column = start_column; character_column < string_length; ++character_column) {
        const uint32_t next_visual_column = string[character_column] == u'\t' ? nextTabStop(visual_column, tab_width) : visual_column + 1;
        const auto character_width = static_cast<int32_t>(next_visual_column - visual_column) * font_advance;
        if (targetX < pen_position_x + character_width / 2) {
            return character_column;
        }
        pen_position_x += character_width;
        visual_column = next_visual_column;
    }

    // Past the end of the line: clamp to eol
    return string_length;
}

void Editor::placeCursorAtPixel(CursorContext &context, const ViewState &viewState, const FrameMetrics &metrics, const int32_t x, const int32_t y, const bool extendSelection) {
    const auto line_height = m_theme.getLineHeight();
    const auto border_size = m_theme.getDimension(DimensionId::BorderSize);
    const auto line_count = context.cursor.getLineCount();

    // A line row spans [position_y + line * line_height - scroll_y, +line_height): invert it.
    // Clicks above the first line clamp to it, clicks past the last line clamp to the last one.
    const auto picked_line = (y - viewState.getPositionY() + context.scroll.y) / line_height;
    const auto line = static_cast<uint32_t>(std::clamp(picked_line, int64_t{0}, static_cast<int64_t>(line_count) - 1));

    // Pixel to column, in the same geometry drawText lays the glyphs out with
    const auto text_start_x = viewState.getPositionX() + metrics.margin_width + border_size;
    const auto column = columnAtPixel(context, line, x - text_start_x + context.scroll.x);

    if (extendSelection && line == m_drag_line && column == m_drag_column) {
        // The pointer stayed in the cell the drag already placed: nothing changed, don't redraw
        return;
    }
    m_drag_line = line;
    m_drag_column = column;

    // Same bookkeeping as the `move` command: reset the search statistics, arm or disarm the
    // selection before moving so the anchor stays at the pressed cell, stick to the new column
    // and scroll the indicator back into view (which auto-scrolls when a drag leaves the view).
    context.search.resetMatches();
    context.cursor.activateSelection(extendSelection);
    context.cursor.setPosition(line, column);
    context.stick.index = context.cursor.getColumn();
    context.scroll.follow_indicator = true;
    context.wants_redraw = true;
}

void Editor::onMouseDown(CursorContext &context, ViewState &viewState, const int32_t x, const int32_t y) {
    // The mouse deliberately leaves the keyboard focus untouched: it only manipulates buffer state
    // (caret, selection, scroll), so a pending prompt interaction survives while the user inspects
    // the buffer before answering.
    const auto metrics = computeFrameMetrics(context, viewState);
    const auto border_size = m_theme.getDimension(DimensionId::BorderSize);

    // Get the vew geometry
    const auto position_x = viewState.getPositionX();
    const auto position_y = viewState.getPositionY();
    const auto width = viewState.getWidth();
    const auto height = viewState.getHeight();

    // The vertical track is drawn full-height and covers the corner square: test it first
    if (metrics.v_bar_width > 0 && x >= position_x + width - metrics.v_bar_width) {
        const auto view_h = static_cast<int64_t>(height - metrics.h_bar_height);
        if (view_h <= 0) {
            return;
        }

        const auto bar = computeScrollbarMetrics(position_y, view_h, contentHeight(context), context.scroll.y);
        if (y >= bar.thumb_origin && y < bar.thumb_origin + bar.thumb_size) {
            // Grab the thumb: motion maps back to the vertical scroll offset
            m_mouse_drag = MouseDrag::VerticalThumb;
            m_drag_grab = y;
            m_drag_scroll = context.scroll.y;
        } else {
            // Track press: jump the scroll by one page toward the click
            const auto max_scroll = std::max(int64_t{0}, bar.content_size - view_h);
            const auto page = view_h;
            context.scroll.y = std::clamp(context.scroll.y + (y < bar.thumb_origin ? -page : page), int64_t{0}, max_scroll);
            context.wants_redraw = true;
        }
        return;
    }

    const auto text_start_x = position_x + metrics.margin_width + border_size;
    if (metrics.h_bar_height > 0 && y >= position_y + height - metrics.h_bar_height && x >= text_start_x) {
        const auto view_w = static_cast<int64_t>(width - metrics.margin_width - border_size - metrics.v_bar_width);
        if (view_w <= 0) {
            return;
        }

        const auto bar = computeScrollbarMetrics(text_start_x, view_w, contentWidth(metrics.longest_line_length), context.scroll.x);
        if (x >= bar.thumb_origin && x < bar.thumb_origin + bar.thumb_size) {
            // Grab the thumb: motion maps back to the horizontal scroll offset
            m_mouse_drag = MouseDrag::HorizontalThumb;
            m_drag_grab = x;
            m_drag_scroll = context.scroll.x;
        } else {
            // Track press: jump the scroll by one page toward the click
            const auto max_scroll = std::max(int64_t{0}, bar.content_size - view_w);
            const auto page = view_w;
            context.scroll.x = std::clamp(context.scroll.x + (x < bar.thumb_origin ? -page : page), int64_t{0}, max_scroll);
            context.wants_redraw = true;
        }
        return;
    }

    // Text (or margin) press: place the caret at the clicked character with the selection
    // disarmed, and arm a text drag so motion extends a selection anchored at this cell
    m_mouse_drag = MouseDrag::Text;
    placeCursorAtPixel(context, viewState, metrics, x, y, false);
}

void Editor::onMouseMotion(CursorContext &context, ViewState &viewState, const int32_t x, const int32_t y) {
    switch (m_mouse_drag) {
        case MouseDrag::Text: {
            // Extend the selection from the pressed cell to the cell under the pointer
            const auto metrics = computeFrameMetrics(context, viewState);
            placeCursorAtPixel(context, viewState, metrics, x, y, true);
        }
        break;
        case MouseDrag::VerticalThumb: {
            const auto metrics = computeFrameMetrics(context, viewState);
            const auto view_h = static_cast<int64_t>(viewState.getHeight() - metrics.h_bar_height);
            if (view_h <= 0) {
                break;
            }

            const auto bar = computeScrollbarMetrics(viewState.getPositionY(), view_h, contentHeight(context), context.scroll.y);
            applyThumbDrag(context.scroll.y, y, bar, context);
        }
        break;
        case MouseDrag::HorizontalThumb: {
            const auto metrics = computeFrameMetrics(context, viewState);
            const auto border_size = m_theme.getDimension(DimensionId::BorderSize);
            const auto view_w = static_cast<int64_t>(viewState.getWidth() - metrics.margin_width - border_size - metrics.v_bar_width);
            if (view_w <= 0) {
                break;
            }

            const auto text_start_x = viewState.getPositionX() + metrics.margin_width + border_size;
            const auto bar = computeScrollbarMetrics(text_start_x, view_w, contentWidth(metrics.longest_line_length), context.scroll.x);
            applyThumbDrag(context.scroll.x, x, bar, context);
        }
        break;
        case MouseDrag::None:
        break;
    }
}

void Editor::onMouseUp(CursorContext &context, ViewState &viewState, const int32_t x, const int32_t y) {
    // Releasing the button ends whatever drag was in progress; a plain click without motion
    // leaves no selection armed since the press already disarmed it
    (void) context;
    (void) viewState;
    (void) x;
    (void) y;
    m_mouse_drag = MouseDrag::None;
}

void Editor::registerTabToSpaceCVar() const {
    m_command_controller.registerCvar(u"tab_to_space", m_is_tab_to_space, nullptr);
}

void Editor::registerShowScrollbarCVar() const {
    m_command_controller.registerCvar(u"show_scrollbar", m_show_scrollbar, nullptr);
}
