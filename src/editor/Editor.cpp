#include "Editor.h"

#include <iostream>
#include <algorithm>
#include <ranges>
#include <utf8.h>

#include "../core/theme/DimensionId.h"


Editor::Editor(CommandManager& commandManager, Theme& theme, QuadProgram& quadProgram, QuadBuffer& quadBuffer)
    : View(commandManager, theme, quadProgram, quadBuffer),
      m_longest_line_cache(0, 0, 0),
      m_is_tab_to_space(std::make_shared<CVarBool>(true)) {}

void Editor::create() {
    registerCvars();
}

void Editor::destroy() {
}

void Editor::render(const HighLighter& highLighter, const Cursor& cursor, EditorState& viewState, const float dt) {
    const auto position_x = viewState.getPositionX();
    const auto position_y = viewState.getPositionY();
    const auto width = viewState.getWidth();
    const auto height = viewState.getHeight();

    const auto& border_color = m_theme.getColor(ColorId::Border);
    const auto& background_color = m_theme.getColor(ColorId::EditorBackground);
    const auto& margin_color = m_theme.getColor(ColorId::MarginBackground);
    const auto& line_number_color = m_theme.getColor(ColorId::LineNumber);
    const auto indicator_width = m_theme.getDimension(DimensionId::IndicatorWidth);
    const auto border_size = m_theme.getDimension(DimensionId::BorderSize);
    const auto tab_to_space = m_theme.getDimension(DimensionId::TabToSpace);
    const auto padding_width = m_theme.getDimension(DimensionId::PaddingWidth);

    const auto line_height = m_theme.getLineHeight();
    const auto font_descender = m_theme.getFontDescender();
    const auto font_advance = m_theme.getFontAdvance();

    const auto cursor_line =  static_cast<int32_t>(cursor.getLine());
    const auto cursor_column =  static_cast<int32_t>(cursor.getColumn());
    const auto cursor_line_count = static_cast<int32_t>(cursor.getLineCount());
    const auto cursor_string = cursor.getString();

    const auto string_cursor_line_count = utf8::utf8to16(std::to_string(cursor_line_count));
    const auto cursor_line_count_width = m_theme.measure(string_cursor_line_count, true);
    const auto margin_width = padding_width + cursor_line_count_width + padding_width;
    const auto cursor_text_start_x = position_x + margin_width + border_size;

    // Follow or scroll to the said position. scrollX and scrollY and followIndicator are eventually updated outside (by reference)
    updateLongestLineCache(cursor, cursor_line_count, cursor_line, cursor_string);
    if (viewState.isFollowingIndicator()) {
        const auto scroll_x = viewState.getScrollX();
        const auto scroll_y = viewState.getScrollY();
        const auto cursor_string_to_indicator = cursor_string.substr(0, cursor_column);
        const auto indicator_x = m_theme.measure(cursor_string_to_indicator, false);
        const auto indicator_y = line_height * cursor_line;

        // Vertical scroll
        if (indicator_y < scroll_y) {
            viewState.setScrollY(indicator_y);
        } else if (indicator_y > height + scroll_y - line_height) {
            viewState.setScrollY(indicator_y - (height - line_height));
        }

        // Horizontal scroll
        if (indicator_x < scroll_x) {
            viewState.setScrollX(indicator_x);
        } else if (indicator_x > width - margin_width - border_size + scroll_x) {
            viewState.setScrollX(indicator_x - width + margin_width + border_size + indicator_width);
        }
    } else {
        // Update max-scroll values
        const auto scroll_x = viewState.getScrollX();
        const auto scroll_y = viewState.getScrollY();
        const auto max_scroll_y = cursor_line_count * line_height - height;
        const auto max_scroll_x = m_longest_line_cache.width - (width - margin_width - border_size - indicator_width);
        viewState.setScrollY(std::clamp(scroll_y, 0, max_scroll_y < 0 ? 0 : max_scroll_y));
        viewState.setScrollX(std::clamp(scroll_x, 0, max_scroll_x < 0 ? 0 : max_scroll_x));
    }

    // Get updated scroll values
    const auto scroll_x = viewState.getScrollX();
    const auto scroll_y = viewState.getScrollY();

    // Map the buffer at offset 1024, keep a variable to know how many quads we have before the cursor text
    auto quads_count_before_text = 0u;
    m_quad_buffer.map(2048, 8192-2048);

    // begin background margin
    m_quad_buffer.insert(
        position_x,
        position_y,
        margin_width, height,
        margin_color.red, margin_color.green, margin_color.blue, margin_color.alpha);

    // begin margin right border
    m_quad_buffer.insert(
        static_cast<int16_t>(position_x + margin_width),
        static_cast<int16_t>(position_y),
        border_size, height,
        border_color.red, border_color.green, border_color.blue, border_color.alpha);

    // begin background editor
    m_quad_buffer.insert(
        static_cast<int16_t>(position_x + margin_width + border_size),
        static_cast<int16_t>(position_y),
        width - margin_width - border_size, height,
        background_color.red, background_color.green, background_color.blue, background_color.alpha);

    // begin text with line numbers, starting from line index until the end of the cursor
    const auto first_line_in_viewport = scroll_y / line_height;
    const auto line_scroll_offset_y = -scroll_y % line_height;
    auto pen_position_x = 0;
    auto pen_position_y = line_scroll_offset_y + position_y + line_height + font_descender;
    auto line_index = first_line_in_viewport;
    while (line_index < cursor_line_count) {
        if (line_index >= 0) {
            const auto string_line_number = utf8::utf8to16(std::to_string(line_index + 1));
            const auto string_line_number_width = m_theme.measure(string_line_number, true);

            pen_position_x = position_x + padding_width + cursor_line_count_width - string_line_number_width;
            for (const auto c : string_line_number) {
                const auto& character = m_theme.getCharacter(c);
                m_quad_buffer.insert(
                    static_cast<int16_t>(pen_position_x + character.bearing_x),
                    static_cast<int16_t>(pen_position_y - character.bearing_y),
                    character.width, character.height,
                    character.texture_s, character.texture_t, character.texture_p, character.texture_q, character.layer,
                    line_number_color.red, line_number_color.green, line_number_color.blue, line_number_color.alpha);

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

    // Keep trace of the quads count in the quad buffer
    quads_count_before_text = m_quad_buffer.getCount();

    // Begin text with cursor text, starting from line index until the end of the cursor
    // We will need to check if we draw a selected text
    pen_position_y = line_scroll_offset_y + position_y + line_height + font_descender;
    line_index = first_line_in_viewport;
    while (line_index < cursor_line_count) {
        if (line_index >= 0) {
            // Get the string at line_index
            const auto string = cursor.getString(line_index);
            const auto string_length = string.length();
            const auto is_cursor_line = cursor_line == line_index;

            if (is_cursor_line) {
                // Begin current line bg
                const auto& line_background_color = m_theme.getColor(ColorId::LineBackground);
                m_quad_buffer.insert(
                    static_cast<int16_t>(cursor_text_start_x),
                    static_cast<int16_t>(pen_position_y - line_height - font_descender),
                    width, line_height,
                    line_background_color.red, line_background_color.green, line_background_color.blue, line_background_color.alpha);
            }

            if (const auto& selected_range = cursor.getSelectedRange(); selected_range.has_value()) {
                // Check if the selected range is in the viewport
                const auto& selected_background_color = m_theme.getColor(ColorId::SelectedTextBackground);
                if (selected_range->line_start == line_index && selected_range->line_end == line_index) {
                    // The selection start / end on the same line. Select only a range of text.
                    const auto selected_text = string.substr(selected_range->column_start, selected_range->column_end - selected_range->column_start);
                    const auto selected_text_width = m_theme.measure(selected_text, true);
                    const auto selection_start_x = m_theme.measure(string.substr(0, selected_range->column_start), true);
                    m_quad_buffer.insert(
                        static_cast<int16_t>(cursor_text_start_x - scroll_x + selection_start_x),
                        static_cast<int16_t>(pen_position_y - line_height - font_descender),
                        selected_text_width, line_height,
                        selected_background_color.red, selected_background_color.green, selected_background_color.blue, selected_background_color.alpha);
                } else if (line_index == selected_range->line_start) {
                    // First line of selected text, the selection starts at column until the end of the text area.
                    const auto selection_start_x = m_theme.measure(string.substr(0, selected_range->column_start), true);
                    m_quad_buffer.insert(
                        static_cast<int16_t>(cursor_text_start_x - scroll_x + selection_start_x),
                        static_cast<int16_t>(pen_position_y - line_height - font_descender),
                        width - selection_start_x, line_height,
                        selected_background_color.red, selected_background_color.green, selected_background_color.blue, selected_background_color.alpha);
                } else if (line_index == selected_range->line_end) {
                    // Last line of selected text, the selection starts at the margin border, until the end column.
                    const auto selected_text = string.substr(0, selected_range->column_end);
                    const auto selected_text_width = m_theme.measure(selected_text, true);
                    m_quad_buffer.insert(
                        static_cast<int16_t>(cursor_text_start_x - scroll_x),
                        static_cast<int16_t>(pen_position_y - line_height - font_descender),
                        selected_text_width, line_height,
                        selected_background_color.red, selected_background_color.green, selected_background_color.blue, selected_background_color.alpha);
                } else if (line_index > selected_range->line_start && line_index < selected_range->line_end) {
                    // In between two selected lines, the selection takes the whole width
                    m_quad_buffer.insert(
                        static_cast<int16_t>(cursor_text_start_x),
                        static_cast<int16_t>(pen_position_y - line_height - font_descender),
                        width, line_height,
                        selected_background_color.red, selected_background_color.green, selected_background_color.blue, selected_background_color.alpha);
                }
            }

            // Start drawing a new line, starting from the first character visible in the viewport, until the end of the string
            auto cursor_position_x = cursor_text_start_x - scroll_x;
            pen_position_x = cursor_position_x;

            for (auto character_column = 0; character_column < string_length; ++character_column) {
                if (pen_position_x > position_x + width) {
                    // Nothing more is visible
                    break;
                }

                const auto token_id = highLighter.getHighLightAtPosition(line_index, character_column);
                const auto& character_color = m_theme.getColor(token_id);
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
                            const auto& character = m_theme.getCharacter(c);
                            m_quad_buffer.insert(
                                static_cast<int16_t>(pen_position_x + character.bearing_x),
                                static_cast<int16_t>(pen_position_y - character.bearing_y),
                                character.width, character.height,
                                character.texture_s, character.texture_t, character.texture_p, character.texture_q, character.layer,
                                character_color.red, character_color.green, character_color.blue, character_color.alpha);
                        }
                        pen_position_x += font_advance;
                    break;
                }

                if (is_cursor_line && character_column < cursor_column) {
                    cursor_position_x = pen_position_x;
                }
            }

            if (is_cursor_line) {
                // begin indicator
                const auto& indicator_color = m_theme.getColor(ColorId::CursorIndicator);
                m_quad_buffer.insert(
                    static_cast<int16_t>(cursor_position_x),
                    static_cast<int16_t>(pen_position_y - line_height - font_descender),
                    indicator_width, line_height,
                    indicator_color.red, indicator_color.green, indicator_color.blue, indicator_color.alpha);
            }
        }

        pen_position_y += line_height;
        if (pen_position_y >= position_y + height + line_height + font_descender) {
            // There is no need to continue at this point, all remaining lines are hidden
            break;
        }

        ++line_index;
    }
    m_quad_buffer.unmap();

    // Draw backgrounds and line number
    glScissor(
        position_x,
        m_window_height - position_y - height,
        width,
        height);

    m_quad_program.draw(
        2048,
        quads_count_before_text);

    // Draw cursor text
    glScissor(
        position_x + margin_width + border_size,
        m_window_height - position_y - height,
        width - margin_width - border_size,
        height);

    m_quad_program.draw(
        2048 + quads_count_before_text,
        m_quad_buffer.getCount() - quads_count_before_text);
}

bool Editor::onKeyDown(const HighLighter& highLighter, Cursor& cursor, EditorState& viewState, const SDL_Keycode keyCode, const uint16_t keyModifier) const {
    (void) keyModifier;
    switch (keyCode) {
        case SDLK_UP:
            viewState.setFollowIndicator(true);
            cursor.activateSelection(keyModifier & KMOD_SHIFT);
            cursor.moveUp();
            return true;
        case SDLK_DOWN:
            viewState.setFollowIndicator(true);
            cursor.activateSelection(keyModifier & KMOD_SHIFT);
            cursor.moveDown();
        return true;
        case SDLK_LEFT:
            viewState.setFollowIndicator(true);
            cursor.activateSelection(keyModifier & KMOD_SHIFT);
            cursor.moveLeft();
        return true;
        case SDLK_RIGHT:
            viewState.setFollowIndicator(true);
            cursor.activateSelection(keyModifier & KMOD_SHIFT);
            cursor.moveRight();
        return true;
        case SDLK_HOME:
            viewState.setFollowIndicator(true);
            cursor.activateSelection(keyModifier & KMOD_SHIFT);
            cursor.moveToStartOfLine();
        return true;
        case SDLK_END:
            viewState.setFollowIndicator(true);
            cursor.activateSelection(keyModifier & KMOD_SHIFT);
            cursor.moveToEndOfLine();
        return true;
        case SDLK_PAGEUP: {
            viewState.setFollowIndicator(true);
            const auto line_amount = m_theme.getDimension(DimensionId::PageUpDown);
            cursor.activateSelection(keyModifier & KMOD_SHIFT);
            cursor.pageUp(line_amount);
        }
        return true;
        case SDLK_PAGEDOWN: {
            viewState.setFollowIndicator(true);
            const auto line_amount = m_theme.getDimension(DimensionId::PageUpDown);
            cursor.activateSelection(keyModifier & KMOD_SHIFT);
            cursor.pageDown(line_amount);
        }
        return true;
        case SDLK_RETURN: {
            viewState.setFollowIndicator(true);
            if (const auto& edit = cursor.eraseSelection(); edit.has_value()) {
                // Any new inputs deactivate the selection and cut the previously selected text before inserting the new input
                highLighter.edit(edit.value());
                cursor.setPosition(edit->new_end.line, edit->new_end.column);
                cursor.activateSelection(false);
            }

            const auto& edit = cursor.newLine();
            highLighter.edit(edit);
        }
        return true;
        case SDLK_BACKSPACE: {
            viewState.setFollowIndicator(true);
            if (const auto& selection = cursor.eraseSelection(); selection.has_value()) {
                // Any new inputs deactivate the selection and cut the previously selected text before inserting the new input
                highLighter.edit(selection.value());
                cursor.setPosition(selection->new_end.line, selection->new_end.column);
                cursor.activateSelection(false);
            } else if (const auto& edit = cursor.eraseLeft(); edit.has_value()) {
                highLighter.edit(edit.value());
            }
        }
        return true;
        case SDLK_DELETE: {
            viewState.setFollowIndicator(true);
            if (const auto& selection = cursor.eraseSelection(); selection.has_value()) {
                // Any new inputs deactivate the selection and cut the previously selected text before inserting the new input
                highLighter.edit(selection.value());
                cursor.setPosition(selection->new_end.line, selection->new_end.column);
                cursor.activateSelection(false);
            } else if (const auto& edit = cursor.eraseRight(); edit.has_value()) {
                highLighter.edit(edit.value());
            }
        }
        return true;
        case SDLK_TAB: {
            viewState.setFollowIndicator(true);
            if (const auto& selection = cursor.eraseSelection(); selection.has_value()) {
                // Any new inputs deactivate the selection and cut the previously selected text before inserting the new input
                highLighter.edit(selection.value());
                cursor.setPosition(selection->new_end.line, selection->new_end.column);
                cursor.activateSelection(false);
            }

            if (m_is_tab_to_space->m_value) {
                // We have to replace the tab character by x amount of space character
                const auto space_amount = m_theme.getDimension(DimensionId::TabToSpace);
                const auto& edit = cursor.insert(std::u16string(space_amount, u' '));
                highLighter.edit(edit);
            } else {
                const auto& edit = cursor.insert(u"\t");
                highLighter.edit(edit);
            }
        }
        return true;
        default:
        return false;
    }
}

void Editor::onTextInput(const HighLighter& highLighter, Cursor &cursor, EditorState &viewState, const char *text) const {
    auto utf8_text = std::string(text);
    if (!utf8::is_valid(utf8_text)) {
        // throw std::runtime_error("Invalid UTF-8 text: " + utf8_text);
        // Let's replace it by the diamond interrogation mark instead of throwing an exception
        std::cerr << "Invalid UTF-8 text: " << utf8_text << std::endl;

        utf8_text = utf8::replace_invalid(utf8_text);
    }

    if (const auto& selection = cursor.eraseSelection(); selection.has_value()) {
        // Any new inputs deactivate the selection and cut the previously selected text before inserting the new input
        highLighter.edit(selection.value());
        cursor.setPosition(selection->new_end.line, selection->new_end.column);
        cursor.activateSelection(false);
    }

    const auto utf16_text = utf8::utf8to16(utf8_text);
    const auto& edit = cursor.insert(utf16_text);
    highLighter.edit(edit);
    viewState.setFollowIndicator(true);
}

void Editor::updateLongestLineCache(const Cursor &cursor, const int32_t cursorLineCount, const int32_t cursorLine, const std::u16string_view cursorString) {
    // Find the longest line in the buffer and calculate its width
    const auto cursor_string_width = m_theme.measure(cursorString, false);
    if (cursorLineCount != m_longest_line_cache.count
        || cursorLine == m_longest_line_cache.index && cursor_string_width < m_longest_line_cache.width) {
        m_longest_line_cache.count = cursorLineCount;
        m_longest_line_cache.index = 0;
        m_longest_line_cache.width = 0;
        for (auto line = 0; line < cursorLineCount; ++line) {
            const auto string = cursor.getString(line);
            const auto string_width = m_theme.measure(string, false);
            if (string_width > m_longest_line_cache.width) {
                m_longest_line_cache.width = string_width;
                m_longest_line_cache.index = line;
            }
        }
    } else if (cursorLine != m_longest_line_cache.index
        && cursor_string_width > m_longest_line_cache.width) {

        m_longest_line_cache.index = cursorLine;
        m_longest_line_cache.width = cursor_string_width;
    } else if (cursorLine == m_longest_line_cache.index
        && cursor_string_width > m_longest_line_cache.width) {

        m_longest_line_cache.width = cursor_string_width;
    }
}

void Editor::registerCvars() const {
    // Register a cvar to change the font size. It needs a callback.
    m_command_manager.registerCvar("tab_to_space", m_is_tab_to_space);
}
