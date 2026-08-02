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
    (void) previousArgs;
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
    (void) payload;
    if (args.size() != 1) {
        return u"Usage: buffer <next|prev|close|name>";
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
        // Closes without warning: no dirty-flag tracking exists yet.
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
