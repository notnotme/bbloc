#include "Prompt.h"

#include <format>
#include <iostream>
#include <utf8.h>

#include "../core/theme/ColorId.h"
#include "../core/theme/DimensionId.h"


Prompt::Prompt(CommandManager& commandManager,Theme& theme, QuadProgram& quadProgram, QuadBuffer& quadBuffer)
    : View(commandManager, theme, quadProgram, quadBuffer) {}

void Prompt::render(const HighLighter &highLighter, const PromptCursor &cursor, PromptState &viewState, float dt) {
    const auto position_x = viewState.getPositionX();
    const auto position_y = viewState.getPositionY();
    const auto width = viewState.getWidth();
    const auto height = viewState.getHeight();

    const auto& border_color = m_theme.getColor(ColorId::Border);
    const auto& prompt_text_color = m_theme.getColor(ColorId::PromptText);
    const auto& input_text_color = m_theme.getColor(ColorId::PromptInputText);
    const auto& background_color = m_theme.getColor(ColorId::PromptBackground);
    const auto border_size = m_theme.getDimension(DimensionId::BorderSize);
    const auto tab_to_space = m_theme.getDimension(DimensionId::TabToSpace);
    const auto padding_width = m_theme.getDimension(DimensionId::PaddingWidth);

    const auto line_height = m_theme.getLineHeight();
    const auto font_descender = m_theme.getFontDescender();
    const auto font_advance = m_theme.getFontAdvance();
    const auto cursor_column = cursor.getColumn();

    m_quad_buffer.map(1024, 1024);
    // background
    m_quad_buffer.insert(
        static_cast<int16_t>(position_x),
        static_cast<int16_t>(position_y + border_size),
        width,
        height - border_size,
        background_color.red, background_color.green, background_color.blue, background_color.alpha);

    // top border
    m_quad_buffer.insert(
        position_x,
        position_y,
        width,
        border_size,
        border_color.red, border_color.green, border_color.blue, border_color.alpha);

    // prompt text
    const auto pen_position_y = position_y + border_size + line_height + font_descender;
    auto pen_position_x = position_x + padding_width;
    for (const auto c : viewState.getPromptText()) {
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
                    prompt_text_color.red, prompt_text_color.green, prompt_text_color.blue, prompt_text_color.alpha);

                pen_position_x += font_advance;
            break;
        }

        // todo: fixme there is no reason to have this here, cursor name will be truncated if too long
        if (pen_position_x > position_x + width) {
            break;
        }
    }

    // buffer text
    const auto string = cursor.getString();
    const auto string_length = string.length();
    auto cursor_position_x = pen_position_x;
    for (auto character_column = 0; character_column < string_length; ++character_column) {
        switch (const auto c = string[character_column]) {
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
                    input_text_color.red, input_text_color.green, input_text_color.blue, input_text_color.alpha);

                pen_position_x += font_advance;
                break;
        }

        if (character_column < cursor_column) {
            cursor_position_x = pen_position_x;
        }

        // todo: fixme there is no reason to have this here, cursor name will be truncated if too long
        if (pen_position_x > position_x + width) {
            break;
        }
    }

    // begin indicator
    if (viewState.getRunningState() == PromptState::RunningState::Running) {
        const auto& indicator_color = m_theme.getColor(ColorId::CursorIndicator);
        const auto indicator_width = m_theme.getDimension(DimensionId::IndicatorWidth);
        m_quad_buffer.insert(
            static_cast<int16_t>(cursor_position_x),
            static_cast<int16_t>(pen_position_y - line_height - font_descender),
            indicator_width, line_height,
            indicator_color.red, indicator_color.green, indicator_color.blue, indicator_color.alpha);
    }

    // When completion or history is active, show it on the right side
    const auto is_navigating_history = viewState.isNavigatingHistory();
    const auto completion_count = is_navigating_history ? viewState.getHistoryCount() : viewState.getCompletionCount();
    const auto completion_index =  is_navigating_history ? viewState.getHistoryIndex() : viewState.getCompletionIndex();
    const auto string_active_completion = utf8::utf8to16(std::format("{}/{}", completion_index + 1, completion_count));
    if (completion_count > 0) {
        const auto active_completion_width = m_theme.measure(string_active_completion, true);
        pen_position_x = position_x + width - padding_width - active_completion_width;
        for (const auto c : string_active_completion) {
            const auto& character = m_theme.getCharacter(c);
            m_quad_buffer.insert(
                static_cast<int16_t>(pen_position_x + character.bearing_x),
                static_cast<int16_t>(pen_position_y - character.bearing_y),
                character.width, character.height,
                character.texture_s, character.texture_t, character.texture_p, character.texture_q, character.layer,
                prompt_text_color.red, prompt_text_color.green, prompt_text_color.blue, prompt_text_color.alpha);

            pen_position_x += font_advance;
        }
    }

    m_quad_buffer.unmap();

    // Draw
    glScissor(
        position_x,
        m_window_height - position_y - height,
        width,
        height);

    m_quad_program.draw(1024, m_quad_buffer.getCount());
}

bool Prompt::onKeyDown(const HighLighter &highLighter, PromptCursor &cursor, PromptState &viewState, SDL_Keycode keyCode, uint16_t keyModifier) const {
    switch (keyCode) {
        case SDLK_UP:
            if (viewState.getHistoryCount() > 0) {
                viewState.clearCompletions();
                const auto command = viewState.previousHistory();
                cursor.clear();
                cursor.insert(command);
                return true;
            }
            //viewState.follow_indicator = true;
        return true;
        case SDLK_DOWN:
            if (viewState.getHistoryCount() > 0) {
                viewState.clearCompletions();
                const auto command = viewState.nextHistory();
                cursor.clear();
                cursor.insert(command);
                return true;
            }
            //viewState.follow_indicator = true;
        return true;
        case SDLK_LEFT:
            //viewState.follow_indicator = true;
            cursor.moveLeft();
        return true;
        case SDLK_RIGHT:
            //viewState.follow_indicator = true;
            cursor.moveRight();
        return true;
        case SDLK_HOME:
            //viewState.follow_indicator = true;
            cursor.moveToStart();
        return true;
        case SDLK_END:
            //viewState.follow_indicator = true;
            cursor.moveToEnd();
        return true;
        case SDLK_BACKSPACE:
            // Reset completions as soon as the user typed a new text
            viewState.clearCompletions();
            //viewState.follow_indicator = true;
            cursor.eraseLeft();
        return true;
        case SDLK_DELETE:
            // Reset completions as soon as the user typed a new text
            viewState.clearCompletions();
            //viewState.follow_indicator = true;
            cursor.eraseRight();
        return true;
        case SDLK_TAB: {
            const auto input = cursor.getString();
            const auto tokens = CommandManager::tokenize(input);
            // Reset the history index if we were browsing
            viewState.clearHistoryIndex();
            if (viewState.getCompletionCount() > 0) {
                // The viewState completion list is not empty, loop inside
                const auto completion = (keyModifier & KMOD_SHIFT) ? viewState.previousCompletion() : viewState.nextCompletion();
                cursor.clear();
                cursor.insert(completion);
                return true;
            }

            if (const auto& feedback = m_command_manager.getCommandFeedback(); feedback.has_value()) {
                // If a feedback is active, try to gather arguments
                m_command_manager.getFeedbackCompletion([&viewState](const std::u16string_view& completion) {
                    viewState.addCompletion(completion);
                });
            } else {
                // Find command name, argument, and argument index from the user input
                const auto utf8_command_name = tokens.empty() ? "" : utf8::utf16to8(tokens.front());
                const auto utf8_argument_to_complete = tokens.size() <= 1 ? "" : utf8::utf16to8(tokens.back());
                const auto argument_index = std::max(0, static_cast<int32_t>(tokens.size() - 2));

                // Try to complete commands arguments first, if the command name is incomplete, this will return an empty list
                m_command_manager.getArgumentsCompletion(utf8_command_name, argument_index, utf8_argument_to_complete,
                    [&](const std::string_view& completion) {
                        const auto completion_str = std::u16string(tokens[0]).append(u" ").append(utf8::utf8to16(completion));
                        viewState.addCompletion(completion_str);
                    });

                if (viewState.getCompletionCount() == 0 && tokens.size() <= 1) {
                    // Auto-complete commands names
                    m_command_manager.getCommandCompletions(utf8_command_name,
                        [&](const std::string_view& completion) {
                            viewState.addCompletion(utf8::utf8to16(completion));
                        });
                }
            }

            // Populate the viewState list and insert the first item in the cursor
            const auto completion_count = viewState.getCompletionCount();
            if (completion_count > 0) {
                viewState.sortCompletions();

                const auto completion = viewState.getCurrentCompletion();
                cursor.clear();
                cursor.insert(completion);

                if (completion_count == 1) {
                    // If we got only a single result, then append a space at the end of the command line
                    // and clear the actual completion, so the next argument completion can occur.
                    cursor.insert(u" ");
                    viewState.clearCompletions();
                }

                return true;
            }
        }
        return false;
        case SDLK_ESCAPE:
            // Reset completions, history index, and feedback if the user quit the prompt
            m_command_manager.clearCommandFeedback();
            viewState.clearCompletions();
            viewState.clearHistoryIndex();
            viewState.setRunningState(PromptState::RunningState::Idle);
        return true;
        case SDLK_RETURN:
            // Reset completions if the user quit the prompt
            viewState.clearCompletions();
            viewState.setRunningState(PromptState::RunningState::Validated);
        return true;
        default:
            return false;
    }
}

void Prompt::onTextInput(const HighLighter &highLighter, PromptCursor &cursor, PromptState &viewState, const char *text) const {
    auto utf8_text = std::string(text);
    if (!utf8::is_valid(utf8_text)) {
        // throw std::runtime_error("Invalid UTF-8 text: " + utf8_text);
        // Let's replace it by the diamond interrogation mark instead of throwing an exception
        std::cerr << "Invalid UTF-8 text: " << utf8_text << std::endl;

        utf8_text = utf8::replace_invalid(utf8_text);
    }

    const auto utf16_text = utf8::utf8to16(utf8_text);
    cursor.insert(utf16_text);

    // Reset completions and history index as soon as the user typed a new text
    viewState.clearCompletions();
    viewState.clearHistoryIndex();

    //todo: viewState.follow_indicator = true;
}
