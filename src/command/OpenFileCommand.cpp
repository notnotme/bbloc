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
#include "OpenFileCommand.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <system_error>

#include <utf8.h>

#include "../core/CommandManager.h"


OpenFileCommand::OpenFileCommand(CursorContextManager &contextManager)
    : m_context_manager(contextManager) {}

void OpenFileCommand::provideAutoComplete(const std::span<const std::u16string_view> previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) previousArgs;
    if (argumentIndex != 0) {
        // Only auto-complete the first argument (path)
        return;
    }

    CommandManager::getPathCompletions(input, false, itemCallback);
}

std::optional<std::u16string> OpenFileCommand::run(CursorContext &payload, const std::span<const std::u16string_view> args) {
    if (args.empty()) {
        // From the prompt the filename is mandatory; from the editor, ask for it interactively.
        if (payload.from_prompt) {
            return u"Usage: open <filename>";
        }

        payload.command_feedback = requestPathArgument(u"open ", u"open", payload.command_runner,
            [](const std::u16string_view input, const AutoCompleteCallback &itemCallback) {
                CommandManager::getPathCompletions(input, false, itemCallback);
            });

        return std::nullopt;
    }

    if (args.size() > 1) {
        return u"Usage: open <filename>";
    }

    // Get the path of the file and read it fully before touching any buffer,
    // so a failed load leaves no half-open buffer behind.
    const auto path = utf8::utf16to8(args[0]);
    auto content = std::u16string();
    if (auto error = readFile(path, content)) {
        return error;
    }

    // Load in place when the active context is pristine (no name, empty buffer);
    // otherwise the file opens in its own new context.
    auto &active = m_context_manager.active();
    const auto is_pristine = active.cursor.getName().empty()
        && active.cursor.getLineCount() == 1
        && active.cursor.getString(0).empty();

    if (is_pristine) {
        loadInto(active, path, content);
    } else {
        loadInto(m_context_manager.createContext(), path, content);

        // The new context was appended last: make it the active one.
        m_context_manager.activate(m_context_manager.getCount() - 1);
    }

    // In case the command is bound to a key, it will eventually needs a redraw the views.
    payload.wants_redraw = true;
    return std::nullopt;
}

std::optional<std::u16string> OpenFileCommand::readFile(const std::string &path, std::u16string &outContent) {
    auto error_code = std::error_code();
    const auto is_regular_file = std::filesystem::is_regular_file(path, error_code);
    auto ifs = std::ifstream(path, std::ios::in);
    if (!ifs || !is_regular_file) {
        // That file cannot be opened
        return std::u16string(u"Could not open ").append(utf8::utf8to16(path)).append(u".");
    }

    // Start to count the lines from 1
    auto line_count = 1u;
    // Stores temporary line and the whole text.
    auto line = std::string();
    auto all_line = std::u16string();

    // A UTF-16 unit count never exceeds the UTF-8 byte count, so the file size is a tight upper bound.
    if (const auto file_size = std::filesystem::file_size(path, error_code); !error_code) {
        all_line.reserve(static_cast<size_t>(file_size));
    }

    while (getline(ifs, line)) {
        if (!line.empty() && line.back() == '\r') {
            // Strip the carriage return of CRLF line endings
            line.pop_back();
        }

        if (const auto &end_it = utf8::find_invalid(line.begin(), line.end()); end_it != line.end()) {
            // Invalid sequence: stop reading the file, before the buffer is touched
            const auto line_count_str = std::to_string(line_count);
            const auto utf16_line_count_str = utf8::utf8to16(line_count_str);
            return std::u16string(u"Invalid UTF-8 encoding detected at line ").append(utf16_line_count_str);
        }
        // Convert to utf16 in place; this cannot throw, the line was validated by find_invalid above.
        utf8::utf8to16(line.begin(), line.end(), std::back_inserter(all_line));
        if (!ifs.eof() && !ifs.fail()) {
            // After the first insert, line ends with \n, but not the last
            all_line.append(u"\n");
        }

        ++line_count;
    }
    ifs.close();

    outContent = std::move(all_line);
    return std::nullopt;
}

void OpenFileCommand::loadInto(CursorContext &target, const std::string &path, const std::u16string_view content) {
    // The whole content is validated: it is now safe to replace the buffer and switch the highlight mode.
    const auto &edit_clear = target.cursor.clear();
    target.highlighter.edit(edit_clear);

    const auto file_extension = std::filesystem::path(path).extension().string();
    target.highlighter.setMode(file_extension);

    // Insert all text at once.
    const auto &edit_insert = target.cursor.insert(content);
    target.highlighter.edit(edit_insert);

    // Set cursor name, reset position and discard the undo history of the previous buffer.
    target.cursor.setName(path);
    target.cursor.setPosition(0, 0);
    target.cursor.clearHistory();
    target.scroll.follow_indicator = true;
    target.stick.active = false;
    target.stick.index = 0;
    target.wants_redraw = true;
}
