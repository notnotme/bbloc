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
#include "ExecCommand.h"

#include <filesystem>
#include <fstream>
#include <system_error>

#include <utf8.h>

#include "../core/CommandManager.h"


void ExecCommand::provideAutoComplete(const std::span<const std::u16string_view> previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) previousArgs;
    if (argumentIndex != 0) {
        // Only auto-complete the first argument (path)
        return;
    }

    CommandManager::getPathCompletions(input, false, itemCallback);
}

std::optional<std::u16string> ExecCommand::run(CursorContext &payload, const std::span<const std::u16string_view> args) {
    if (args.size() != 1) {
        return u"Usage: exec <filename>";
    }

    // A script executing itself (or a cycle of scripts) would otherwise recurse forever.
    if (m_recursion_depth >= MAX_RECURSION_DEPTH) {
        return u"Max exec recursion depth reached.";
    }

    // Get the path of the file and tries to open the file at this location
    const auto path = utf8::utf16to8(args[0]);
    auto error_code = std::error_code();
    const auto is_regular_file = std::filesystem::is_regular_file(path, error_code);
    auto ifs = std::ifstream(path, std::ios::in);
    if (!ifs || !is_regular_file) {
        // That file cannot be opened
        return std::u16string(u"Could not open ").append(args[0]).append(u".");
    }

    // This will store the command list to run and the current line of the file that we are reading.
    auto command_list = std::vector<std::u16string>();
    auto line = std::string();

    auto line_count = 1;
    while (getline(ifs, line)) {
        if (const auto &end_it = utf8::find_invalid(line.begin(), line.end()); end_it != line.end()) {
            // Invalid sequence: stop the command list
            const auto line_count_str = std::to_string(line_count);
            const auto utf16_line_count_str = utf8::utf8to16(line_count_str);
            return std::u16string(u"Invalid UTF-8 encoding detected at line ").append(utf16_line_count_str);
        }

        // Convert to utf16 then append to the cursor
        if (!line.starts_with("#")) {
            // If the line starts with "#", this is a comment, otherwise this is a command.
            command_list.push_back(utf8::utf8to16(line));
        }
        ++line_count;
    }
    ifs.close();

    ++m_recursion_depth;
    for (const auto &command : command_list) {
        // fixme?: At this point, any feedback needed will interrupt the command list execution
        // fixme!: This is not well tested at all.
        payload.command_runner.runCommand(command, false);
    }
    --m_recursion_depth;

    return std::nullopt;
}
