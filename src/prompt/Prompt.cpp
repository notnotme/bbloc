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
#include "Prompt.h"

#include <algorithm>
#include <format>
#include <iostream>
#include <utf8.h>

#include "../ApplicationWindow.h"
#include "../core/theme/ColorId.h"
#include "../core/theme/DimensionId.h"
#include "../core/theme/TabStop.h"


Prompt::Prompt(GlobalRegistry<CursorContext> &commandController, Theme &theme, QuadProgram &quadProgram)
    : View(commandController, theme, quadProgram) {}

void Prompt::render(CursorContext &context, PromptState &viewState, QuadBuffer &quadBuffer, float dt) {
    const auto batch_start = quadBuffer.beginBatch(ApplicationWindow::PROMPT_DEFAULT_QUAD_COUNT);
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

bool Prompt::onKeyDown(CursorContext &context, PromptState &viewState, const SDL_Keycode keyCode, const uint16_t keyModifier) const {
    switch (keyCode) {
        case SDLK_RETURN: {
            // runCommand can also update the prompt state, set the prompt to Idle before running the command.
            viewState.setRunningState(PromptState::RunningState::Idle);
            viewState.setPromptText(PromptState::PROMPT_READY);
            viewState.clearCompletions();
            viewState.clearHistoryIndex();

            // The command may close the active buffer ("buffer close"), destroying `context`.
            // The runner and the prompt cursor are owned by ApplicationWindow, not by the context,
            // so binding them here keeps them usable after the dispatch.
            auto &command_runner = context.command_runner;
            auto &prompt_cursor = context.prompt_cursor;

            // The return value of runCommand can be ignored in this use case.
            // Clearing stays after the call: ApplicationWindow::runCommand reads the pending
            // feedback answer out of the prompt cursor.
            const auto prompt_command = prompt_cursor.getString();
            command_runner.runCommand(prompt_command, true);
            prompt_cursor.clear();
        }
        return true;
        case SDLK_ESCAPE:
            // Set the prompt state to Idle, then the command processing logic will take care of the rest.
            // Escape also cancels a pending feedback, so the next command is not consumed as its answer.
            viewState.setRunningState(PromptState::RunningState::Idle);
            viewState.setPromptText(PromptState::PROMPT_READY);
            viewState.clearCompletions();
            viewState.clearHistoryIndex();
            context.prompt_cursor.clear();
            context.command_feedback.reset();
            context.focus_target = FocusTarget::Editor;
        return true;
        case SDLK_BACKSPACE:
            // Reset completions as soon as the user typed a new text
            viewState.clearCompletions();
            //viewState.follow_indicator = true;
            context.prompt_cursor.eraseLeft();
        return true;
        case SDLK_DELETE:
            // Reset completions as soon as the user typed a new text
            viewState.clearCompletions();
            //viewState.follow_indicator = true;
            context.prompt_cursor.eraseRight();
        return true;
        default:
        return false;
    }
}

void Prompt::onTextInput(CursorContext &context, PromptState &viewState, const char *text) const {
    auto utf8_text = std::string(text);
    if (!utf8::is_valid(utf8_text)) {
        // throw std::runtime_error("Invalid UTF-8 text: " + utf8_text);
        // Let's replace it by the diamond interrogation mark instead of throwing an exception
        std::cerr << "Invalid UTF-8 text: " << utf8_text << std::endl;

        utf8_text = utf8::replace_invalid(utf8_text);
    }

    const auto utf16_text = utf8::utf8to16(utf8_text);
    if (viewState.getCompletionCount() > 0 && utf16_text == u"/" && context.prompt_cursor.getString().ends_with(u'/')) {
        // Typing '/' on a folder candidate accepts it instead of inserting a duplicate slash,
        // so the next completion descends into the folder.
        viewState.clearCompletions();
        viewState.clearHistoryIndex();
        return;
    }

    context.prompt_cursor.insert(utf16_text);

    // Reset completions and history index as soon as the user typed a new text
    viewState.clearCompletions();
    viewState.clearHistoryIndex();

    //todo: viewState.follow_indicator = true;
}

void Prompt::drawBackground(QuadBuffer &quadBuffer, const PromptState &viewState) const {
    // Get the vew geometry
    const auto position_x = viewState.getPositionX();
    const auto position_y = viewState.getPositionY();
    const auto width = viewState.getWidth();
    const auto height = viewState.getHeight();

    // Need some variables
    const auto &border_color = m_theme.getColor(ColorId::Border);
    const auto &background_color = m_theme.getColor(ColorId::InfoBarBackground);
    const auto border_size = m_theme.getDimension(DimensionId::BorderSize);

    drawQuad(quadBuffer, position_x, position_y + border_size, width, height - border_size, background_color);
    drawQuad(quadBuffer, position_x, position_y, width, border_size, border_color);
}

void Prompt::drawText(QuadBuffer &quadBuffer, const CursorContext &context, const PromptState &viewState) const {
    // Get the vew geometry
    const auto position_x = viewState.getPositionX();
    const auto position_y = viewState.getPositionY();
    const auto width = viewState.getWidth();

    // Keep some variable that frequently needed
    const auto &prompt_text_color = m_theme.getColor(ColorId::PromptText);
    const auto border_size = m_theme.getDimension(DimensionId::BorderSize);
    const uint32_t tab_width = static_cast<uint32_t>(std::max(m_theme.getDimension(DimensionId::TabToSpace), 1));
    const auto padding_width = m_theme.getDimension(DimensionId::PaddingWidth);
    const auto line_height = m_theme.getLineHeight();
    const auto font_descender = m_theme.getFontDescender();
    const auto font_advance = m_theme.getFontAdvance();

    // Write only 1 line of text on the y axis, make it const. The x axis will change.
    const auto pen_position_y = position_y + border_size + line_height + font_descender;
    auto pen_position_x = position_x + padding_width;

    // One visual column spans the prompt label and the input text, anchored at the padding
    // origin, so the tab-stop grid stays continuous across the two strings
    uint32_t visual_column = 0;

    // Draw the prompt text
    for (const auto prompt_text = viewState.getPromptText(); const auto c : prompt_text) {
        switch (c) {
            case ' ' :
                pen_position_x += font_advance;
                ++visual_column;
            break;
            case '\t' : {
                // A tab advances the pen to the next tab stop, 1 to tab_width columns away
                const uint32_t next_tab_stop = nextTabStop(visual_column, tab_width);
                pen_position_x += font_advance * static_cast<int32_t>(next_tab_stop - visual_column);
                visual_column = next_tab_stop;
            }
            break;
            default:
                const auto &character = m_theme.getCharacter(c);
                drawCharacter(quadBuffer, pen_position_x, pen_position_y, character, prompt_text_color);
                pen_position_x += font_advance;
                ++visual_column;
            break;
        }

        // todo: fixme there is no reason to have this here, cursor name will be truncated if too long
        if (pen_position_x > position_x + width) {
            break;
        }
    }

    // Draw the prompt cursor text
    const auto &input_text_color = m_theme.getColor(ColorId::PromptInputText);
    const auto string = context.prompt_cursor.getString();
    const auto string_length = static_cast<uint32_t>(string.length());
    const auto cursor_column = context.prompt_cursor.getColumn();

    // Needs to keep track of the cursor position indicator
    auto cursor_position_x = pen_position_x;
    for (uint32_t character_column = 0; character_column < string_length; ++character_column) {
        switch (const auto c = string[character_column]) {
            case ' ' :
                pen_position_x += font_advance;
                ++visual_column;
            break;
            case '\t' : {
                // A tab advances the pen to the next tab stop, 1 to tab_width columns away
                const uint32_t next_tab_stop = nextTabStop(visual_column, tab_width);
                pen_position_x += font_advance * static_cast<int32_t>(next_tab_stop - visual_column);
                visual_column = next_tab_stop;
            }
            break;
            default:
                const auto &character = m_theme.getCharacter(c);
                drawCharacter(quadBuffer, pen_position_x, pen_position_y, character, input_text_color);
                pen_position_x += font_advance;
                ++visual_column;
            break;
        }

        if (character_column < cursor_column) {
            // Update cursor position
            cursor_position_x = pen_position_x;
        }

        // todo: fixme there is no reason to have this here, cursor name will be truncated if too long
        if (pen_position_x > position_x + width) {
            break;
        }
    }

    // Draw the cursor position indicator
    if (viewState.getRunningState() == PromptState::RunningState::Running) {
        const auto &indicator_color = m_theme.getColor(ColorId::CursorIndicator);
        const auto indicator_width = m_theme.getDimension(DimensionId::IndicatorWidth);
        drawQuad(quadBuffer, cursor_position_x, pen_position_y - line_height - font_descender, indicator_width, line_height, indicator_color);
    }

    // Draw a right-aligned "index/total" counter. History and completion counters only exist while
    // the prompt is focused; the search counter persists so it stays visible with focus in the editor.
    auto indicator_index = 0;
    auto indicator_count = 0;
    if (viewState.isNavigatingHistory()) {
        indicator_index = viewState.getHistoryIndex();
        indicator_count = viewState.getHistoryCount();
    } else if (viewState.getCompletionCount() > 0) {
        indicator_index = viewState.getCompletionIndex();
        indicator_count = viewState.getCompletionCount();
    } else if (context.search.match_count > 0) {
        indicator_index = context.search.match_index;
        indicator_count = context.search.match_count;
    }

    if (indicator_count > 0) {
        const auto string_indicator = utf8::utf8to16(std::format("{}/{}", indicator_index + 1, indicator_count));
        // The measure lives in 64-bit content space; a short counter string always fits the screen
        const auto indicator_text_width = static_cast<int32_t>(m_theme.measure(string_indicator));
        pen_position_x = position_x + width - padding_width - indicator_text_width;
        for (const auto c : string_indicator) {
            const auto &character = m_theme.getCharacter(c);
            drawCharacter(quadBuffer, pen_position_x, pen_position_y, character, prompt_text_color);
            pen_position_x += font_advance;
        }
    }
}

