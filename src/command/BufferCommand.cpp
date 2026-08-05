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
#include "BufferCommand.h"

#include <array>
#include <format>

#include <utf8.h>


BufferCommand::BufferCommand(CursorContextManager &contextManager)
    : m_context_manager(contextManager) {}

void BufferCommand::provideAutoComplete(const std::span<const std::u16string_view> previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    if (argumentIndex == 1 && previousArgs.size() == 1 && previousArgs[0] == u"close") {
        // The second argument of "close" can only be the flag skipping the confirmation
        constexpr auto force_flag = std::u16string_view(u"-f");
        if (force_flag.starts_with(input)) {
            itemCallback(force_flag);
        }
        return;
    }

    if (argumentIndex != 0) {
        // Only auto-complete the first argument (action or buffer name)
        return;
    }

    static constexpr auto actions = std::array<std::u16string_view, 3> { u"next", u"prev", u"close" };
    for (const auto &action : actions) {
        if (action.starts_with(input)) {
            itemCallback(action);
        }
    }

    // Offer the names of the open buffers too, for a direct switch
    for (size_t index = 0; index < m_context_manager.getCount(); ++index) {
        const auto name = m_context_manager.get(index).cursor.getName();
        if (name.empty()) {
            // A scratch buffer has no name to complete
            continue;
        }

        if (const auto utf16_name = utf8::utf8to16(name); utf16_name.starts_with(input)) {
            itemCallback(utf16_name);
        }
    }
}

std::optional<std::u16string> BufferCommand::run(CursorContext &payload, const std::span<const std::u16string_view> args) {
    // The switch happens through the manager; the payload stays the invoking context.
    // Only "close" takes a second argument, the "-f" flag skipping the unsaved-changes prompt.
    const auto is_forced_close = args.size() == 2 && args[0] == u"close" && args[1] == u"-f";
    if (args.size() != 1 && !is_forced_close) {
        return u"Usage: buffer <next|prev|close [-f]|name>";
    }

    if (args[0] == u"next") {
        m_context_manager.next();
        return statusMessage();
    }

    if (args[0] == u"prev") {
        m_context_manager.prev();
        return statusMessage();
    }

    if (args[0] == u"close") {
        // A buffer holding unsaved changes only closes after an explicit confirmation.
        if (m_context_manager.active().cursor.isModified() && !is_forced_close) {
            payload.command_feedback = CommandFeedback {
                .prompt_message = u"Buffer has unsaved changes, close ? [y/N]: ",
                // This reuses the same command, but with "-f" argument to skip this prompt.
                .command_string = u"buffer close -f",
                .on_complete_callback = [](const std::u16string_view input, const AutoCompleteCallback &itemCallback) {
                    (void) input;
                    itemCallback(u"n");
                    itemCallback(u"y");
                },
                .on_validate_callback = [&](const std::u16string_view input, const std::u16string_view command) -> std::optional<std::u16string> {
                    if (input == u"y" || input == u"Y") {
                        payload.command_runner.runCommand(command, true);
                        return std::nullopt;
                    }
                    return std::nullopt;
                }
            };

            return std::nullopt;
        }

        m_context_manager.close();
        return statusMessage();
    }

    // Any other argument is a direct switch to the open buffer with that name
    if (const auto index = m_context_manager.indexOf(utf8::utf16to8(args[0]))) {
        m_context_manager.activate(*index);
        return statusMessage();
    }

    return std::u16string(u"No such buffer: ").append(args[0]);
}

std::u16string BufferCommand::statusMessage() {
    auto &context = m_context_manager.active();
    const auto name = context.cursor.getName();
    const auto display_name = name.empty() ? std::string_view("Untitled") : name;
    return utf8::utf8to16(std::format("buffer {}/{}: {}", context.buffer_index, context.buffer_count, display_name));
}
