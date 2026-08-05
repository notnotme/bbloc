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
#include "AutoCompleteCommand.h"

#include <ranges>

#include "../core/CommandManager.h"


const U16StringMap<AutoCompleteCommand::Direction> AutoCompleteCommand::DIRECTION_MAP = {
    { u"forward", Direction::Forward },
    { u"backward", Direction::Backward }
};

AutoCompleteCommand::AutoCompleteCommand(PromptState &promptState)
    : m_prompt_state(promptState) {}

void AutoCompleteCommand::provideAutoComplete(const std::span<const std::u16string_view> previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) previousArgs;
    if (argumentIndex != 0) {
        // Only auto-complete the first argument (direction)
        return;
    }

    for (const auto &item : std::views::keys(DIRECTION_MAP)) {
        if (item.starts_with(input)) {
            itemCallback(item);
        }
    }
}

std::optional<std::u16string> AutoCompleteCommand::run(CursorContext &payload, const std::span<const std::u16string_view> args) {
    if (args.size() > 1) {
        return u"Expected 0 or 1 argument.";
    }

    // Determine which direction to use for the completion
    const auto direction = !args.empty()
        // User decide.
        ? mapDirection(args[0])
        // Goes forward by default.
        : Direction::Forward;

    if (direction == Direction::Unknown) {
        // Return an error if we can't determine the direction
        return std::u16string(u"Unknown direction argument: ").append(args[0]);
    }

    // Get and tokenize the input string
    const auto input = payload.prompt_cursor.getString();
    auto tokens = std::vector<std::u16string_view>();
    CommandManager::tokenize(input, tokens);

    // Reset the history index if we were browsing it
    m_prompt_state.clearHistoryIndex();
    if (m_prompt_state.getCompletionCount() > 0) {
        // The viewState completion list is not empty, loop inside
        const auto completion = direction == Direction::Forward
            ? m_prompt_state.nextCompletion()
            : m_prompt_state.previousCompletion();

        payload.prompt_cursor.clear();
        payload.prompt_cursor.insert(completion);
        payload.wants_redraw = true;
        return std::nullopt;
    }

    auto has_open_quote = false;
    if (payload.command_feedback) {
        // If feedback is active, try to gather arguments
        if (payload.command_feedback->on_complete_callback) {
            // The whole answer is a single argument: strip its surrounding quotes before the lookup.
            const auto quote_is_open = input.starts_with(u'"');
            auto lookup = std::u16string_view(input);
            if (quote_is_open) {
                lookup.remove_prefix(1);
                if (lookup.ends_with(u'"')) {
                    lookup.remove_suffix(1);
                }
            }

            // One buffer reused across candidates; addCompletion copies the view into its own storage.
            auto completion_buffer = std::u16string();
            payload.command_feedback->on_complete_callback(lookup, [&](const std::u16string_view item) {
                // Candidates containing a space must be quoted, unless the user already opened a quote
                const auto needs_quote = !quote_is_open && item.find(u' ') != std::u16string_view::npos;
                has_open_quote = quote_is_open || needs_quote;

                completion_buffer.clear();
                if (has_open_quote) {
                    completion_buffer.push_back(u'"');
                }

                completion_buffer.append(item);
                m_prompt_state.addCompletion(completion_buffer);
            });
        }
    } else {
        // The user is completing the last token if nothing, or only its closing quote, follows it
        const auto *input_end = input.data() + input.size();
        const auto *last_token_end = tokens.empty() ? input_end : tokens.back().data() + tokens.back().size();
        const auto completing_last_token = !tokens.empty()
            && (last_token_end == input_end || (last_token_end + 1 == input_end && *last_token_end == u'"'));

        // A single token still being typed is the command name itself, not an argument
        const auto completing_command_name = tokens.size() <= 1 && completing_last_token;

        // Find command name, argument to complete, and argument index from the user input
        const auto command_name = tokens.empty() ? u"" : tokens.front();
        const auto argument_to_complete = completing_last_token ? tokens.back() : std::u16string_view(u"");
        const auto argument_index = static_cast<int32_t>(tokens.size() <= 1 || !completing_last_token
            // Zero or one token, or a new argument is starting = last argument index
            ? tokens.size() - 1
            // Otherwise, the last token is being completed and its index is tokens.size() - 2
            : tokens.size() - 2);

        // Reconstitute the left part of the input, which completions do not replace.
        auto reconstituted_command = std::u16string();
        if (completing_last_token) {
            // Everything before the completed token, keeping an eventual opening quote.
            reconstituted_command = input.substr(0, tokens.back().data() - input.data());
        } else {
            // A new argument is starting, keep the whole input and make sure it is space-terminated.
            reconstituted_command = input;
            if (!input.ends_with(u' ')) {
                reconstituted_command.append(u" ");
            }
        }

        // Gather the arguments written before the one being completed, excluding the command name.
        auto previous_args = std::span<const std::u16string_view>();
        if (completing_last_token && tokens.size() >= 2) {
            previous_args = std::span(tokens).subspan(1, tokens.size() - 2);
        } else if (!completing_last_token && !tokens.empty()) {
            previous_args = std::span(tokens).subspan(1);
        }

        // Try to complete commands arguments first, if the command name is incomplete, this will return an empty list
        if (!completing_command_name) {
            // Each candidate is appended after the invariant left part, reusing the buffer;
            // addCompletion copies the view into its own storage.
            const auto quote_is_open = reconstituted_command.ends_with(u'"');
            const auto base_length = reconstituted_command.size();
            payload.command_runner.getArgumentsCompletions(command_name, previous_args, argument_index, argument_to_complete,
                [&](const std::u16string_view completion) {
                    // Candidates containing a space must be quoted, unless the user already opened a quote
                    const auto needs_quote = !quote_is_open && completion.find(u' ') != std::u16string_view::npos;
                    has_open_quote = quote_is_open || needs_quote;

                    reconstituted_command.resize(base_length);
                    if (needs_quote) {
                        reconstituted_command.push_back(u'"');
                    }

                    reconstituted_command.append(completion);
                    m_prompt_state.addCompletion(reconstituted_command);
                });
        }

        if (m_prompt_state.getCompletionCount() == 0 && tokens.size() <= 1) {
            // Auto-complete commands names then
            payload.command_runner.getCommandCompletions(command_name,
                [&](const std::u16string_view completion) {
                    m_prompt_state.addCompletion(completion);
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
            // A unique folder match is left open (no space, no closing quote), so the next completion descends into it.
            if (!completion.ends_with(u'/')) {
                if (has_open_quote) {
                    payload.prompt_cursor.insert(u"\"");
                }

                // A feedback answer is a single argument: only a full command line gets the argument-separating space.
                if (!payload.command_feedback) {
                    payload.prompt_cursor.insert(u" ");
                }
            }

            m_prompt_state.clearCompletions();
        }

        payload.wants_redraw = true;
    }

    return std::nullopt;
}

AutoCompleteCommand::Direction AutoCompleteCommand::mapDirection(const std::u16string_view direction) {
    if (const auto &mapped_direction = DIRECTION_MAP.find(direction); mapped_direction != DIRECTION_MAP.end()) {
        return mapped_direction->second;
    }

    return Direction::Unknown;
}
