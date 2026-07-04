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

#include <functional>
#include <string>
#include <string_view>

#include "AutoCompleteCallback.h"
#include "FeedbackCallback.h"


/**
 * @brief Represents a feedback interaction triggered by a command.
 *
 * Used when a command requires user confirmation or additional input after initial execution
 * (e.g., "Are you sure? [y/n]"). This structure holds the prompt message, the command string
 * to run next, an optional completion provider, and a callback to handle the user's input.
 */
struct CommandFeedback final {
    std::u16string prompt_message;                  ///< Prompt message displayed to the user.
    std::u16string command_string;                  ///< Command associated with the feedback.
    std::function<void(std::u16string_view input, const AutoCompleteCallback &itemCallback)> on_complete_callback; ///< Optional provider computing completions from the current input.
    FeedbackCallback on_validate_callback;          ///< Callback to run after receiving user input.
};


#endif //COMMAND_FEEDBACK_H
