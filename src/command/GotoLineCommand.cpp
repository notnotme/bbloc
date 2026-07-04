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
#include "GotoLineCommand.h"

#include <algorithm>

#include <utf8/cpp17.h>


void GotoLineCommand::provideAutoComplete(const std::vector<std::u16string_view> &previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) previousArgs;
    (void) argumentIndex;
    (void) input;
    (void) itemCallback;
    // No-op
}

std::optional<std::u16string> GotoLineCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    if (args.empty()) {
        // From the prompt the line number is mandatory; from the editor, ask for it interactively.
        if (payload.from_prompt) {
            return u"Expected 1 argument.";
        }

        payload.command_feedback = CommandFeedback {
            .prompt_message = u"goto_line ",
            .command_string = u"goto_line",
            .completions_list = {},
            .on_validate_callback = [&](const std::u16string_view input, const std::u16string_view command) -> std::optional<std::u16string> {
                payload.command_runner.runCommand(std::u16string(command).append(u" ").append(input), true);
                return std::nullopt;
            }
        };

        return std::nullopt;
    }

    if (args.size() > 1) {
        return u"Expected 1 argument.";
    }

    int32_t requested_line;
    try {
        requested_line = std::stoi(utf8::utf16to8(args[0]));
    } catch (...) {
        return u"Expected a line number.";
    }

    // User lines are 1-based; clamp into range before shifting to the 0-based buffer index.
    const auto line_count = payload.cursor.getLineCount();
    const auto target_line = std::clamp<uint32_t>(requested_line, 1, line_count);
    const auto line_index = target_line - 1;

    payload.cursor.activateSelection(false);
    payload.cursor.setPosition(line_index, 0);
    payload.stick.index = payload.cursor.getColumn();
    payload.scroll.follow_indicator = true;
    payload.wants_redraw = true;

    return std::nullopt;
}
