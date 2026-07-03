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
#ifndef FEEDBACK_CALLBACK_H
#define FEEDBACK_CALLBACK_H

#include <functional>
#include <optional>
#include <string_view>
#include <vector>


/**
 * @brief Type alias for a feedback callback function.
 *
 * A FeedbackCallback is used to process or log the result of a command execution.
 * It receives both the answer/output and the original/modified command as UTF-16 string views.
 * The callback may return an optional modified message or feedback.
 *
 * @param input The output or result of the executed command.
 * @param command The command input to be invoked next if the feedback succeeds.
 * @return An optional UTF-16 string message, such as modified output or additional feedback.
 */
using FeedbackCallback = std::function<
    std::optional<std::u16string>(std::u16string_view input, std::u16string_view command)
>;


#endif //FEEDBACK_CALLBACK_H
