#include "Prompt.h"

#include <format>
#include <iostream>
#include <utf8.h>

#include "../ApplicationWindow.h"
#include "../core/theme/ColorId.h"
#include "../core/theme/DimensionId.h"


Prompt::Prompt(CommandController<CursorContext> &commandController, Theme &theme, QuadProgram &quadProgram, QuadBuffer &quadBuffer)
    : View(commandController, theme, quadProgram, quadBuffer) {}

void Prompt::render(CursorContext &context, PromptState &viewState, float dt) {
    // Map 1024 quads
    m_quad_buffer.map(ApplicationWindow::PROMPT_BUFFER_QUAD_OFFSET, ApplicationWindow::PROMPT_BUFFER_QUAD_COUNT);
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
    m_quad_program.draw(ApplicationWindow::PROMPT_BUFFER_QUAD_OFFSET, m_quad_buffer.getCount());
}

bool Prompt::onKeyDown(CursorContext &context, PromptState &viewState, const SDL_Keycode keyCode, const uint16_t keyModifier) const {
    switch (keyCode) {
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
    context.prompt_cursor.insert(utf16_text);

    // Reset completions and history index as soon as the user typed a new text
    viewState.clearCompletions();
    viewState.clearHistoryIndex();

    //todo: viewState.follow_indicator = true;
}

void Prompt::drawBackground(const PromptState &viewState) const {
    // Get the vew geometry
    const auto position_x = viewState.getPositionX();
    const auto position_y = viewState.getPositionY();
    const auto width = viewState.getWidth();
    const auto height = viewState.getHeight();

    // Need some variables
    const auto &border_color = m_theme.getColor(ColorId::Border);
    const auto &background_color = m_theme.getColor(ColorId::InfoBarBackground);
    const auto border_size = m_theme.getDimension(DimensionId::BorderSize);

    drawQuad(position_x, position_y + border_size, width, height - border_size, background_color);
    drawQuad(position_x, position_y, width, border_size, border_color);
}

void Prompt::drawText(const CursorContext &context, const PromptState &viewState) const {
    // Get the vew geometry
    const auto position_x = viewState.getPositionX();
    const auto position_y = viewState.getPositionY();
    const auto width = viewState.getWidth();

    // Keep some variable that frequently needed
    const auto &prompt_text_color = m_theme.getColor(ColorId::PromptText);
    const auto border_size = m_theme.getDimension(DimensionId::BorderSize);
    const auto tab_to_space = m_theme.getDimension(DimensionId::TabToSpace);
    const auto padding_width = m_theme.getDimension(DimensionId::PaddingWidth);
    const auto line_height = m_theme.getLineHeight();
    const auto font_descender = m_theme.getFontDescender();
    const auto font_advance = m_theme.getFontAdvance();

    // Write only 1 line of text on the y axis, make it const. The x axis will change.
    const auto pen_position_y = position_y + border_size + line_height + font_descender;
    auto pen_position_x = position_x + padding_width;

    // Draw the prompt text
    auto quad_in_buffer = m_quad_buffer.getCount();
    for (const auto prompt_text = viewState.getPromptText(); const auto c : prompt_text) {
        if (quad_in_buffer == ApplicationWindow::PROMPT_BUFFER_QUAD_COUNT) {
            throw std::runtime_error("Not enough quad allowed to render the prompt.");
        }

        switch (c) {
            case ' ' :
                pen_position_x += font_advance;
            break;
            case '\t' :
                pen_position_x += font_advance * tab_to_space;
            break;
            default:
                const auto &character = m_theme.getCharacter(c);
                drawCharacter(pen_position_x, pen_position_y, character, prompt_text_color);
                pen_position_x += font_advance;
                ++quad_in_buffer;
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
    const auto string_length = string.length();
    const auto cursor_column = context.prompt_cursor.getColumn();

    // Needs to keep track of the cursor position indicator
    auto cursor_position_x = pen_position_x;
    for (auto character_column = 0; character_column < string_length; ++character_column) {
        if (quad_in_buffer == ApplicationWindow::PROMPT_BUFFER_QUAD_COUNT) {
            throw std::runtime_error("Not enough quad allowed to render the prompt.");
        }

        switch (const auto c = string[character_column]) {
            case ' ' :
                pen_position_x += font_advance;
            break;
            case '\t' :
                pen_position_x += font_advance * tab_to_space;
            break;
            default:
                const auto &character = m_theme.getCharacter(c);
                drawCharacter(pen_position_x, pen_position_y, character, input_text_color);
                pen_position_x += font_advance;
                ++quad_in_buffer;
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
        if (quad_in_buffer == ApplicationWindow::PROMPT_BUFFER_QUAD_COUNT) {
            throw std::runtime_error("Not enough quad allowed to render the prompt.");
        }
        ++quad_in_buffer;

        const auto &indicator_color = m_theme.getColor(ColorId::CursorIndicator);
        const auto indicator_width = m_theme.getDimension(DimensionId::IndicatorWidth);
        drawQuad(cursor_position_x, pen_position_y - line_height - font_descender, indicator_width, line_height, indicator_color);
    }

    // When completion or history is active, draw it on the right side
    const auto is_navigating_history = viewState.isNavigatingHistory();
    const auto completion_count = is_navigating_history ? viewState.getHistoryCount() : viewState.getCompletionCount();
    const auto completion_index =  is_navigating_history ? viewState.getHistoryIndex() : viewState.getCompletionIndex();
    const auto string_active_completion = utf8::utf8to16(std::format("{}/{}", completion_index + 1, completion_count));
    if (completion_count > 0) {
        const auto active_completion_width = m_theme.measure(string_active_completion, true);
        pen_position_x = position_x + width - padding_width - active_completion_width;
        for (const auto c : string_active_completion) {
            if (quad_in_buffer == ApplicationWindow::PROMPT_BUFFER_QUAD_COUNT) {
                throw std::runtime_error("Not enough quad allowed to render the prompt.");
            }
            ++quad_in_buffer;

            const auto &character = m_theme.getCharacter(c);
            drawCharacter(pen_position_x, pen_position_y, character, prompt_text_color);
            pen_position_x += font_advance;
        }
    }
}

