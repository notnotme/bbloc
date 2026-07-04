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
#include "CommandManager.h"

#include <filesystem>
#include <ranges>
#include <system_error>

#include <utf8.h>


CommandManager::CommandManager()
    : m_cvar_command(std::make_shared<CVarCommand>()) {
    m_commands.insert({ u"cvar", m_cvar_command });
}

void CommandManager::registerCommand(const std::u16string_view name, std::shared_ptr<Command<CursorContext>> command, const bool hidden) {
    const auto &[new_entry, success] = m_commands.insert({ std::u16string(name), std::move(command) });
    if (!success) {
        throw std::runtime_error(std::string("Command already registered: ").append(utf8::utf16to8(name)));
    }

    if (hidden) {
        m_hidden_commands.emplace(name);
    }
}

void CommandManager::registerCvar(const std::u16string_view name, const std::shared_ptr<CVar> cvar, const CVarCallback &callback) {
    m_cvar_command->registerCvar(name, cvar, callback);
}

std::optional<std::u16string> CommandManager::run(CursorContext &payload, const std::span<const std::u16string_view> tokens) {
    if (tokens.empty()) {
        // Nothing to process
        return std::nullopt;
    }

    if (const auto &cmd = m_commands.find(tokens[0]); cmd != m_commands.end()) {
        // Skip the first item in the tokens, as it is the command name and we don't need it
        return cmd->second->run(payload, tokens.subspan(1));
    }

    return std::u16string(u"Unknown command: ").append(tokens[0]);
}

void CommandManager::getCommandCompletions(const std::u16string_view input, const bool includeHidden, const AutoCompleteCallback &itemCallback) {
    for (const auto &name : std::views::keys(m_commands)) {
        if (name.starts_with(input) && (includeHidden || !m_hidden_commands.contains(name))) {
            itemCallback(name);
        }
    }
}

void CommandManager::getArgumentsCompletion(const std::u16string_view command, const std::span<const std::u16string_view> previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) {
    if (const auto &cmd = m_commands.find(command); cmd != m_commands.end()) {
        cmd->second->provideAutoComplete(previousArgs, argumentIndex, input, itemCallback);
    }
}

void CommandManager::getPathCompletions(const std::u16string_view input, const bool foldersOnly, const AutoCompleteCallback &itemCallback) {
    const auto path_input = std::filesystem::path(input);
    const auto path_input_string = path_input.filename().string();
    const auto parent_path = path_input.has_parent_path() ? path_input.parent_path() : ".";

    // Non-throwing filesystem overloads only: completing an unreadable path must never terminate the app.
    auto error_code = std::error_code();
    if (!std::filesystem::is_directory(parent_path, error_code)) {
        return;
    }

    auto iterator = std::filesystem::directory_iterator(parent_path, error_code);
    if (error_code) {
        return;
    }

    for (const auto end = std::filesystem::directory_iterator(); iterator != end; iterator.increment(error_code)) {
        if (error_code) {
            return;
        }

        const auto &entry = *iterator;
        const auto &path = entry.path();
        const auto &filename = path.filename().string();
        if (!filename.starts_with(path_input_string)) {
            continue;
        }

        const auto entry_is_directory = entry.is_directory(error_code);
        if (entry_is_directory || (!foldersOnly && entry.is_regular_file(error_code))) {
            auto complete_path = path.string();
            if (entry_is_directory) {
                // Mark directories so the next completion can descend into them
                complete_path.append("/");
            }

            auto utf16_complete_path = std::u16string();
            try {
                utf16_complete_path = utf8::utf8to16(complete_path);
            } catch (const utf8::exception &) {
                // Skip filenames that are not valid UTF-8
                continue;
            }

            itemCallback(utf16_complete_path);
        }
    }
}

void CommandManager::tokenize(const std::u16string_view input, std::vector<std::u16string_view> &tokens) {
    tokens.clear();
    auto start = 0;
    auto index = 0;
    while (index < input.length()) {
        constexpr auto SPACE_DELIMITER = U' ';
        constexpr auto QUOTE_DELIMITER = U'"';

        // Skip blank spaces
        if (input[index] == SPACE_DELIMITER) {
            ++index;
            continue;
        }

        start = index;
        if (input[index] == QUOTE_DELIMITER) {
            // skip opening quote
            ++start;
            ++index;
            while (index < input.length() && input[index] != QUOTE_DELIMITER) {
                ++index;
            }

            if (index < input.length()) {
                tokens.emplace_back(input.substr(start, index - start));
                // skip closing quote
                ++index;
            } else {
                // Unterminated quote, take until the end
                tokens.emplace_back(input.substr(start));
                break;
            }
        } else {
            // Unquoted word
            while (index < input.size() && input[index] != SPACE_DELIMITER) {
                ++index;
            }
            tokens.emplace_back(input.substr(start, index - start));
        }
    }
}

std::vector<std::u16string_view> CommandManager::split(const std::u16string_view input, const char16_t delimiter) {
    std::vector<std::u16string_view> parts;
    auto start = 0;
    auto index = 0;
    while (index < input.length()) {
        if (input[index] == delimiter) {
            ++index;
            continue;
        }

        start = index;
        while (index < input.size() && input[index] != delimiter) {
            ++index;
        }
        parts.emplace_back(input.substr(start, index - start));
    }

    return parts;
}
