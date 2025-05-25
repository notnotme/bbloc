#include "Editor.h"

#include <iostream>
#include <algorithm>
#include <ranges>
#include <utf8.h>

#include "../ApplicationWindow.h"
#include "../core/theme/DimensionId.h"


Editor::Editor(GlobalRegistry<CursorContext> &commandController, Theme &theme, QuadProgram &quadProgram, QuadBuffer &quadBuffer)
    : View(commandController, theme, quadProgram, quadBuffer),
      m_longest_line_cache(0, 0, 0),
      m_is_tab_to_space(std::make_shared<CVarBool>(true)) {
    // Register cvars
    registerTabToSpaceCVar();
}

void Editor::render(CursorContext &context, ViewState &viewState, const float dt) {
    // Need some variable
    const auto padding_width = m_theme.getDimension(DimensionId::PaddingWidth);
    const auto border_size = m_theme.getDimension(DimensionId::BorderSize);
    const auto cursor_line_count = static_cast<int32_t>(context.cursor.getLineCount());
    const auto string_cursor_line_count = utf8::utf8to16(std::to_string(cursor_line_count));
    const auto cursor_line_count_width = m_theme.measure(string_cursor_line_count, true);
    const auto margin_width = padding_width + cursor_line_count_width + padding_width;

    // Follow or scroll to the said position. scrollX and scrollY and followIndicator are eventually updated outside (by reference)
    updateLongestLineCache(context);
    updateScroll(context, viewState);

    // Get updated scroll values
    const auto scroll_x = context.scroll_x;
    const auto scroll_y = context.scroll_y;

    // Map the buffer at offset 1024, keep a variable to know how many quads we have before the cursor text
    m_quad_buffer.map(ApplicationWindow::EDITOR_BUFFER_QUAD_OFFSET, ApplicationWindow::EDITOR_BUFFER_QUAD_COUNT);
    drawBackground(viewState, margin_width);
    drawMarginText(context, viewState, cursor_line_count_width, scroll_y);

    const auto quads_count_before_text = m_quad_buffer.getCount();
    drawText(context, viewState, scroll_x, scroll_y);
    m_quad_buffer.unmap();

    // Get the vew geometry
    const auto position_x = viewState.getPositionX();
    const auto position_y = viewState.getPositionY();
    const auto width = viewState.getWidth();
    const auto height = viewState.getHeight();

    // Draw backgrounds and line number
    glScissor(position_x, m_window_height - position_y - height, width, height);
    m_quad_program.draw(ApplicationWindow::EDITOR_BUFFER_QUAD_OFFSET, quads_count_before_text);

    // Draw cursor text
    glScissor(position_x + margin_width + border_size, m_window_height - position_y - height, width - margin_width - border_size, height);

    const auto draw_offset = ApplicationWindow::EDITOR_BUFFER_QUAD_OFFSET + quads_count_before_text;
    m_quad_program.draw(draw_offset, m_quad_buffer.getCount() - quads_count_before_text);
}

bool Editor::onKeyDown(CursorContext &context, ViewState &viewState, const SDL_Keycode keyCode, const uint16_t keyModifier) const {
    switch (keyCode) {
        case SDLK_RETURN: {
            context.follow_indicator = true;
            if (const auto &edit = context.cursor.eraseSelection()) {
                // Any new inputs deactivate the selection and cut the previously selected text before inserting the new input
                context.highlighter.edit(edit.value());
                context.cursor.setPosition(edit->new_end.line, edit->new_end.column);
                context.cursor.activateSelection(false);
            }

            const auto &edit = context.cursor.newLine();
            context.highlighter.edit(edit);
        }
        return true;
        case SDLK_BACKSPACE: {
            context.follow_indicator = true;
            if (const auto &selection = context.cursor.eraseSelection()) {
                // Any new inputs deactivate the selection and cut the previously selected text before inserting the new input
                context.highlighter.edit(selection.value());
                context.cursor.setPosition(selection->new_end.line, selection->new_end.column);
                context.cursor.activateSelection(false);
            } else if (const auto &edit = context.cursor.eraseLeft()) {
                context.highlighter.edit(edit.value());
            }
        }
        return true;
        case SDLK_DELETE: {
            context.follow_indicator = true;
            if (const auto &selection = context.cursor.eraseSelection()) {
                // Any new inputs deactivate the selection and cut the previously selected text before inserting the new input
                context.highlighter.edit(selection.value());
                context.cursor.setPosition(selection->new_end.line, selection->new_end.column);
                context.cursor.activateSelection(false);
            } else if (const auto &edit = context.cursor.eraseRight()) {
                context.highlighter.edit(edit.value());
            }
        }
        return true;
        case SDLK_TAB: {
            context.follow_indicator = true;
            if (const auto &selection = context.cursor.eraseSelection()) {
                // Any new inputs deactivate the selection and cut the previously selected text before inserting the new input
                context.highlighter.edit(selection.value());
                context.cursor.setPosition(selection->new_end.line, selection->new_end.column);
                context.cursor.activateSelection(false);
            }

            if (m_is_tab_to_space->m_value) {
                // We have to replace the tab character by x amount of space character
                const auto space_amount = m_theme.getDimension(DimensionId::TabToSpace);
                const auto &edit = context.cursor.insert(std::u16string(space_amount, u' '));
                context.highlighter.edit(edit);
            } else {
                const auto &edit = context.cursor.insert(u"\t");
                context.highlighter.edit(edit);
            }
        }
        return true;
        default:
        return false;
    }
}

void Editor::onTextInput(CursorContext &context, ViewState &viewState, const char *text) const {
    auto utf8_text = std::string(text);
    if (!utf8::is_valid(utf8_text)) {
        // throw std::runtime_error("Invalid UTF-8 text: " + utf8_text);
        // Let's replace it by the diamond interrogation mark instead of throwing an exception
        std::cerr << "Invalid UTF-8 text: " << utf8_text << std::endl;

        utf8_text = utf8::replace_invalid(utf8_text);
    }

    if (const auto &selection = context.cursor.eraseSelection()) {
        // Any new inputs deactivate the selection and cut the previously selected text before inserting the new input
        context.highlighter.edit(selection.value());
        context.cursor.setPosition(selection->new_end.line, selection->new_end.column);
        context.cursor.activateSelection(false);
    }

    const auto utf16_text = utf8::utf8to16(utf8_text);
    const auto &edit = context.cursor.insert(utf16_text);
    context.highlighter.edit(edit);
    context.follow_indicator = true;
}

void Editor::updateLongestLineCache(const CursorContext &context) {
    // Find the longest line in the buffer and calculate its width
    const auto cursor_line_count = context.cursor.getLineCount();
    const auto cursor_line = context.cursor.getLine();
    const auto cursor_string = context.cursor.getString();
    const auto cursor_string_width = m_theme.measure(cursor_string, false);
    if (cursor_line_count != m_longest_line_cache.count
        || cursor_line == m_longest_line_cache.index && cursor_string_width < m_longest_line_cache.width) {
        m_longest_line_cache.count = cursor_line_count;
        m_longest_line_cache.index = 0;
        m_longest_line_cache.width = 0;
        for (auto line = 0; line < cursor_line_count; ++line) {
            const auto string = context.cursor.getString(line);
            const auto string_width = m_theme.measure(string, false);
            if (string_width > m_longest_line_cache.width) {
                m_longest_line_cache.width = string_width;
                m_longest_line_cache.index = line;
            }
        }
    } else if (cursor_line != m_longest_line_cache.index
        && cursor_string_width > m_longest_line_cache.width) {

        m_longest_line_cache.index = cursor_line;
        m_longest_line_cache.width = cursor_string_width;
    } else if (cursor_line == m_longest_line_cache.index
        && cursor_string_width > m_longest_line_cache.width) {

        m_longest_line_cache.width = cursor_string_width;
    }
}

void Editor::updateScroll(CursorContext &context, const ViewState &viewState) const {
    const auto line_height = m_theme.getLineHeight();
    const auto border_size = m_theme.getDimension(DimensionId::BorderSize);
    const auto padding_width = m_theme.getDimension(DimensionId::PaddingWidth);
    const auto indicator_width = m_theme.getDimension(DimensionId::IndicatorWidth);

    const auto cursor_line =  static_cast<int32_t>(context.cursor.getLine());
    const auto cursor_column =  static_cast<int32_t>(context.cursor.getColumn());
    const auto cursor_line_count = static_cast<int32_t>(context.cursor.getLineCount());

    const auto cursor_line_count_string = utf8::utf8to16(std::to_string(cursor_line_count));
    const auto cursor_line_count_width = m_theme.measure(cursor_line_count_string, true);

    const auto width = viewState.getWidth();
    const auto height = viewState.getHeight();
    const auto margin_width = padding_width + cursor_line_count_width + padding_width;

    if (context.follow_indicator) {
        const auto scroll_x = context.scroll_x;
        const auto scroll_y = context.scroll_y;
        const auto cursor_string = context.cursor.getString();
        const auto cursor_string_to_indicator = cursor_string.substr(0, cursor_column);
        const auto indicator_x = m_theme.measure(cursor_string_to_indicator, false);
        const auto indicator_y = line_height * cursor_line;

        // Vertical scroll
        if (indicator_y < scroll_y) {
            context.scroll_y = indicator_y;
        } else if (indicator_y > height + scroll_y - line_height) {
            context.scroll_y = indicator_y - (height - line_height);
        }

        // Horizontal scroll
        if (indicator_x < scroll_x) {
            context.scroll_x = indicator_x;
        } else if (indicator_x > width - margin_width - border_size + scroll_x) {
            context.scroll_x = indicator_x - width + margin_width + border_size + indicator_width;
        }
    } else {
        // Update max-scroll values
        const auto scroll_x = context.scroll_x;
        const auto scroll_y = context.scroll_y;
        const auto max_scroll_y = cursor_line_count * line_height - height;
        const auto max_scroll_x = m_longest_line_cache.width - (width - margin_width - border_size - indicator_width);
        context.scroll_x = std::clamp(scroll_x, 0, max_scroll_x < 0 ? 0 : max_scroll_x);
        context.scroll_y = std::clamp(scroll_y, 0, max_scroll_y < 0 ? 0 : max_scroll_y);
    }
}

void Editor::drawBackground(const ViewState &viewState, const int32_t marginWidth) const {
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
    drawQuad(position_x, position_y, marginWidth, height, margin_color);
    drawQuad(position_x + marginWidth, position_y, border_size, height, border_color);
    drawQuad(position_x + marginWidth + border_size, position_y, width - marginWidth - border_size, height, background_color);
}

void Editor::drawMarginText(const CursorContext &context, const ViewState &viewState, const int32_t lineCountWidth, const int32_t scrollY) const {
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

    // Draw text
    const auto first_line_in_viewport = scrollY / line_height;
    const auto line_scroll_offset_y = -scrollY % line_height;
    auto pen_position_x = 0;
    auto pen_position_y = line_scroll_offset_y + position_y + line_height + font_descender;
    auto line_index = first_line_in_viewport;

    auto quad_in_buffer = m_quad_buffer.getCount();
    while (line_index < cursor_line_count) {
        if (line_index >= 0) {
            const auto string_line_number = utf8::utf8to16(std::to_string(line_index + 1));
            const auto string_line_number_width = m_theme.measure(string_line_number, true);

            pen_position_x = position_x + padding_width + lineCountWidth - string_line_number_width;
            for (const auto c : string_line_number) {
                if (quad_in_buffer == ApplicationWindow::EDITOR_BUFFER_QUAD_COUNT) {
                    throw std::runtime_error("Not enough quad allowed to render the prompt.");
                }

                const auto &character = m_theme.getCharacter(c);
                drawCharacter(pen_position_x, pen_position_y, character, line_number_color);
                pen_position_x += font_advance;
                ++quad_in_buffer;
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

void Editor::drawText(const CursorContext &context, const ViewState &viewState, const int32_t scrollX, const int32_t scrollY) {
    // Get the vew geometry
    const auto position_x = viewState.getPositionX();
    const auto position_y = viewState.getPositionY();
    const auto width = viewState.getWidth();
    const auto height = viewState.getHeight();

    // Need some variable
    const auto indicator_width = m_theme.getDimension(DimensionId::IndicatorWidth);
    const auto border_size = m_theme.getDimension(DimensionId::BorderSize);
    const auto tab_to_space = m_theme.getDimension(DimensionId::TabToSpace);
    const auto padding_width = m_theme.getDimension(DimensionId::PaddingWidth);

    const auto line_height = m_theme.getLineHeight();
    const auto font_descender = m_theme.getFontDescender();
    const auto font_advance = m_theme.getFontAdvance();

    const auto cursor_line =  static_cast<int32_t>(context.cursor.getLine());
    const auto cursor_column =  static_cast<int32_t>(context.cursor.getColumn());
    const auto cursor_line_count = static_cast<int32_t>(context.cursor.getLineCount());

    const auto string_cursor_line_count = utf8::utf8to16(std::to_string(cursor_line_count));
    const auto cursor_line_count_width = m_theme.measure(string_cursor_line_count, true);
    const auto margin_width = padding_width + cursor_line_count_width + padding_width;
    const auto cursor_text_start_x = position_x + margin_width + border_size;

    // Draw text
    const auto first_line_in_viewport = scrollY / line_height;
    const auto line_scroll_offset_y = -scrollY % line_height;
    auto pen_position_x = 0;
    auto pen_position_y = line_scroll_offset_y + position_y + line_height + font_descender;
    auto line_index = first_line_in_viewport;

    auto quad_in_buffer = m_quad_buffer.getCount();
    while (line_index < cursor_line_count) {
        if (line_index >= 0) {
            // Get the string at line_index
            const auto string = context.cursor.getString(line_index);
            const auto string_length = string.length();
            const auto is_cursor_line = cursor_line == line_index;

            if (is_cursor_line) {
                if (quad_in_buffer == ApplicationWindow::EDITOR_BUFFER_QUAD_COUNT) {
                    throw std::runtime_error("Not enough quad allowed to render the prompt.");
                }
                ++quad_in_buffer;

                // Begin current line bg
                const auto &line_background_color = m_theme.getColor(ColorId::LineBackground);
                drawQuad(cursor_text_start_x, pen_position_y - line_height - font_descender, width, line_height, line_background_color);
            }

            if (const auto &selected_range = context.cursor.getSelectedRange()) {
                if (quad_in_buffer > ApplicationWindow::EDITOR_BUFFER_QUAD_COUNT) {
                    throw std::runtime_error("Not enough quad allowed to render the prompt.");
                }

                // Check if the selected range is in the viewport
                const auto &selected_background_color = m_theme.getColor(ColorId::SelectedTextBackground);
                if (selected_range->line_start == line_index && selected_range->line_end == line_index) {
                    ++quad_in_buffer;
                    // The selection start / end on the same line. Select only a range of text.
                    const auto selected_text = string.substr(selected_range->column_start, selected_range->column_end - selected_range->column_start);
                    const auto selected_text_width = m_theme.measure(selected_text, false);
                    const auto selection_start_x = m_theme.measure(string.substr(0, selected_range->column_start), false);
                    drawQuad(cursor_text_start_x - scrollX + selection_start_x, pen_position_y - line_height - font_descender, selected_text_width, line_height, selected_background_color);
                } else if (line_index == selected_range->line_start) {
                    ++quad_in_buffer;
                    // First line of selected text, the selection starts at column until the end of the text area.
                    const auto selection_start_x = m_theme.measure(string.substr(0, selected_range->column_start), false);
                    drawQuad(cursor_text_start_x - scrollX + selection_start_x, pen_position_y - line_height - font_descender, width - selection_start_x, line_height, selected_background_color);
                } else if (line_index == selected_range->line_end) {
                    ++quad_in_buffer;
                    // Last line of selected text, the selection starts at the margin border, until the end column.
                    const auto selected_text = string.substr(0, selected_range->column_end);
                    const auto selected_text_width = m_theme.measure(selected_text, false);
                    drawQuad(cursor_text_start_x - scrollX, pen_position_y - line_height - font_descender, selected_text_width, line_height, selected_background_color);
                } else if (line_index > selected_range->line_start && line_index < selected_range->line_end) {
                    ++quad_in_buffer;
                    // In between two selected lines, the selection takes the whole width
                    drawQuad(cursor_text_start_x, pen_position_y - line_height - font_descender, width, line_height, selected_background_color);
                }
            }

            // Start drawing a new line, starting from the first character visible in the viewport, until the end of the string
            auto cursor_position_x = cursor_text_start_x - scrollX;
            pen_position_x = cursor_position_x;

            for (auto character_column = 0; character_column < string_length; ++character_column) {
                if (pen_position_x > position_x + width) {
                    // Nothing more is visible
                    break;
                }

                if (quad_in_buffer > ApplicationWindow::EDITOR_BUFFER_QUAD_COUNT) {
                    throw std::runtime_error("Not enough quad allowed to render the prompt.");
                }

                switch (const auto c = string[character_column]) {
                    case ' ':
                        pen_position_x += font_advance;
                    break;
                    case '\t':
                        pen_position_x += font_advance * tab_to_space;
                    break;
                    default:
                        if (pen_position_x + font_advance >= position_x) {
                            // Only fetch characters and insert if it could be visible
                            const auto token_id = context.highlighter.getHighLightAtPosition(line_index, character_column);
                            const auto &character = m_theme.getCharacter(c);
                            const auto &character_color = m_theme.getColor(token_id);
                            drawCharacter(pen_position_x, pen_position_y, character, character_color);
                        }
                        pen_position_x += font_advance;
                        ++quad_in_buffer;
                    break;
                }

                if (is_cursor_line && character_column < cursor_column) {
                    cursor_position_x = pen_position_x;
                }
            }

            if (is_cursor_line) {
                if (quad_in_buffer > ApplicationWindow::EDITOR_BUFFER_QUAD_COUNT) {
                    throw std::runtime_error("Not enough quad allowed to render the prompt.");
                }
                ++quad_in_buffer;

                // begin indicator
                const auto &indicator_color = m_theme.getColor(ColorId::CursorIndicator);
                drawQuad(cursor_position_x, pen_position_y - line_height - font_descender, indicator_width, line_height, indicator_color);
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

void Editor::registerTabToSpaceCVar() const {
    // Register a cvar to change the font size. It needs a callback.
    m_command_controller.registerCvar("tab_to_space", m_is_tab_to_space, nullptr);
}
