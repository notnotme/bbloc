#include "AutoCompleteCommand.h"

#include <utf8.h>

AutoCompleteCommand::AutoCompleteCommand(PromptState &promptState)
    : m_prompt_state(promptState) {}

void AutoCompleteCommand::provideAutoComplete(const int32_t argumentIndex, const std::string_view input, const AutoCompleteCallback<char> &itemCallback) const {
    (void) input;
    (void) argumentIndex;
    (void) itemCallback;
    // No-op
}

std::optional<std::u16string> AutoCompleteCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    if (args.size() > 1) {
        return u"Expected 0 or 1 argument.";
    }

    int direction = 0;
    if (!args.empty()) {
        if (args[0] == u"forward") {
            direction = 0;
        } else if (args[0] == u"backward") {
            direction = 1;
        }
    }

    switch (payload.focus_target) {
        case FocusTarget::Prompt: {
            const auto input = payload.prompt_cursor.getString();
            const auto tokens = CommandManager::tokenize(input);
            // Reset the history index if we were browsing
            m_prompt_state.clearHistoryIndex();
            if (m_prompt_state.getCompletionCount() > 0) {
                // The viewState completion list is not empty, loop inside
                const auto completion = direction == 0 ? m_prompt_state.nextCompletion() : m_prompt_state.previousCompletion();
                payload.prompt_cursor.clear();
                payload.prompt_cursor.insert(completion);
                payload.wants_redraw = true;
                return std::nullopt;
            }

            if (payload.command_feedback) {
                // If a feedback is active, try to gather arguments
                for (const auto &item : payload.command_feedback->completions_list) {
                    m_prompt_state.addCompletion(item);
                }
            } else {
                // Find command name, argument to complete, and argument index from the user input
                const auto utf8_command_name = tokens.empty() ? "" : utf8::utf16to8(tokens.front());
                const auto utf8_argument_to_complete = tokens.size() <= 1 ? "" : utf8::utf16to8(tokens.back());
                const auto argument_index = static_cast<int32_t>(tokens.size() <= 1
                    // Zero or one token = command argument
                    ? tokens.size() - 1
                    : input.ends_with(' ')
                        // If the input ends with ' ', argument index is tokens.size() - 1
                        ? tokens.size() - 1
                        // Otherwise, argument index is tokens.size() - 2
                        : tokens.size() - 2);

                // Reconstitute the original input, first find left part of the input.
                auto skip = 2;
                auto left_index = input.rfind(u" \"");
                if (left_index == std::string::npos) {
                    // First tries to find the latest argument with a quote, then fallback to a space.
                    left_index = input.rfind(' ');
                    skip = 1;
                }

                const auto reconstituted_command = left_index == std::string::npos
                    // Nothing was found, we return the entire input + space.
                    ? std::u16string(input).append(u" ")
                    // Return the left part of the input, before the eventually incomplete argument.
                    : input.substr(0, left_index + skip);

                // Try to complete commands arguments first, if the command name is incomplete, this will return an empty list
                payload.command_runner.getArgumentsCompletions(utf8_command_name, argument_index, utf8_argument_to_complete,
                    [&](const std::string_view completion) {
                        // Create a copy of the original strings
                        const auto completion_str = std::u16string(reconstituted_command).append(utf8::utf8to16(completion));
                        m_prompt_state.addCompletion(completion_str);
                    });

                if (m_prompt_state.getCompletionCount() == 0 && tokens.size() <= 1) {
                    // Auto-complete commands names then
                    payload.command_runner.getCommandCompletions(utf8_command_name,
                        [&](const std::string_view completion) {
                            m_prompt_state.addCompletion(utf8::utf8to16(completion));
                        });
                }
            }

            // Populate the viewState list and insert the first item in the cursor
            const auto completion_count = m_prompt_state.getCompletionCount();
            if (completion_count > 0) {
                m_prompt_state.sortCompletions();

                const auto completion = m_prompt_state.getCurrentCompletion();
                payload.prompt_cursor.clear();
                payload.prompt_cursor.insert(completion);

                if (completion_count == 1) {
                    // If we got only a single result, then append a space at the end of the command line
                    // and clear the actual completion, so the next argument completion can occur.
                    payload.prompt_cursor.insert(u" ");
                    m_prompt_state.clearCompletions();
                }

                payload.wants_redraw = true;
                return std::nullopt;
            }
        }
        break;
        case FocusTarget::Editor:
        default:
            // No-op
        break;
    }

    return std::nullopt;
}

bool AutoCompleteCommand::isRunnable(const CursorContext &payload) {
    // Auto complete is only available from the prompt perspective
    return payload.focus_target == FocusTarget::Prompt;
}
