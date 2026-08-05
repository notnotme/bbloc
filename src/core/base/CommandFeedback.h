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
#ifndef COMMAND_FEEDBACK_H
#define COMMAND_FEEDBACK_H

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include "AutoCompleteCallback.h"
#include "CommandRunner.h"
#include "FeedbackCallback.h"


/**
 * @brief Represents a feedback interaction triggered by a command.
 *
 * Used when a command requires user confirmation or additional input after initial execution
 * (e.g., "Are you sure? [y/n]"). This structure holds the prompt message, the command string
 * to run next, an optional completion provider, and a callback to handle the user's input.
 *
 * Every instance carries a distinct id, so a feedback replacing another one can be told apart from
 * the one it replaced. Copies keep the id of the instance they were made from.
 */
struct CommandFeedback final {
    /** Hands out the next identity; the first feedback ever built is 1, so 0 means "none". */
    static inline uint64_t next_id = 0;

    uint64_t id = ++next_id;                        ///< Identity of this feedback, unique across the session.
    std::u16string prompt_message;                  ///< Prompt message displayed to the user.
    std::u16string command_string;                  ///< Command associated with the feedback.
    std::function<void(std::u16string_view input, const AutoCompleteCallback &itemCallback)> on_complete_callback; ///< Optional provider computing completions from the current input.
    FeedbackCallback on_validate_callback;          ///< Callback to run after receiving user input.
};


/**
 * @brief Quotes a prompt answer that a re-tokenization would otherwise split into several arguments.
 *
 * The answer to an interactive prompt is appended to its command and run through the tokenizer
 * again, which cuts on spaces. An answer already holding a quote is passed verbatim instead: it is
 * quoted the way its author meant it to be, and the tokenizer handles it.
 *
 * This rule exists once on purpose. Spelling it per call site is what let a path containing a quote
 * take a different branch depending on which command asked for it.
 *
 * @param value The raw answer typed by the user.
 * @return The answer, wrapped in double quotes when it needs to survive as a single argument.
 */
[[nodiscard]] inline std::u16string quoteArgument(const std::u16string_view value) {
    if (value.find(u' ') == std::u16string_view::npos || value.find(u'"') != std::u16string_view::npos) {
        return std::u16string(value);
    }

    return std::u16string(u"\"").append(value).append(u"\"");
}

/**
 * @brief Builds the feedback asking the user for the single argument a command was invoked without.
 *
 * The answer is appended to the command, which is then re-run from the prompt.
 *
 * @param promptMessage The message shown to the user, usually the command name and a space.
 * @param commandString The command to re-run with the answer appended.
 * @param runner The runner executing the re-run command.
 * @return The feedback to store in the context, carrying a fresh identity.
 */
[[nodiscard]] inline CommandFeedback requestArgument(std::u16string promptMessage, std::u16string commandString, CommandRunner &runner) {
    // The completion provider is empty on purpose: this prompt completes nothing.
    return CommandFeedback{
        .prompt_message = std::move(promptMessage),
        .command_string = std::move(commandString),
        .on_complete_callback = {},
        .on_validate_callback = [&runner](const std::u16string_view input, const std::u16string_view command) -> std::optional<std::u16string> {
            runner.runCommand(std::u16string(command).append(u" ").append(input), true);
            return std::nullopt;
        }
    };
}

/**
 * @brief Builds the feedback asking the user for a path, completed while typing and quoted on validation.
 *
 * @param promptMessage The message shown to the user, usually the command name and a space.
 * @param commandString The command to re-run with the answered path appended.
 * @param runner The runner executing the re-run command.
 * @param pathCompletions The provider listing the paths matching the current input.
 * @return The feedback to store in the context, carrying a fresh identity.
 */
[[nodiscard]] inline CommandFeedback requestPathArgument(std::u16string promptMessage, std::u16string commandString, CommandRunner &runner,
                                                         std::function<void(std::u16string_view input, const AutoCompleteCallback &itemCallback)> pathCompletions) {
    return CommandFeedback{
        .prompt_message = std::move(promptMessage),
        .command_string = std::move(commandString),
        .on_complete_callback = std::move(pathCompletions),
        .on_validate_callback = [&runner](const std::u16string_view input, const std::u16string_view command) -> std::optional<std::u16string> {
            runner.runCommand(std::u16string(command).append(u" ").append(quoteArgument(input)), true);
            return std::nullopt;
        }
    };
}


#endif //COMMAND_FEEDBACK_H
