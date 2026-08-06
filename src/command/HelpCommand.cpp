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
#include "HelpCommand.h"

#include <utf8.h>

#include "../platform/Platform.h"


const std::array<HelpCommand::Section, 6> HelpCommand::SECTIONS = {{
    {u"keyboard",      u"=== Keyboard "},
    {u"mouse",         u"=== Mouse and touch "},
    {u"controller",    u"=== Controller "},
    {u"osk",           u"=== On-screen keyboard "},
    {u"commands",      u"=== Commands "},
    {u"configuration", u"=== Configuration "},
}};

HelpCommand::HelpCommand(CursorContextManager &contextManager)
    : m_context_manager(contextManager) {}

void HelpCommand::provideAutoComplete(const std::span<const std::u16string_view> previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) previousArgs;
    (void) input;
    if (argumentIndex == 0) {
        for (const auto &section : SECTIONS) {
            itemCallback(section.argument);
        }
    }
}

std::optional<std::u16string> HelpCommand::run(CursorContext &payload, const std::span<const std::u16string_view> args) {
    if (args.size() > 1) {
        return u"Expected at most 1 argument.";
    }

    // Resolve the section before opening anything, so a typo fails without side effects.
    const char16_t *heading = nullptr;
    if (args.size() == 1) {
        for (const auto &section : SECTIONS) {
            if (args[0] == section.argument) {
                heading = section.heading;
                break;
            }
        }

        if (heading == nullptr) {
            return std::u16string(u"Unknown section: ").append(args[0]);
        }
    }

    // Dispatch through the open command: buffer reuse comes with it, a second help
    // switches back to the existing manual buffer. The path never contains quotes.
    const auto manual_path = Platform::assetPath("romfs/manual.txt");
    const auto open_command = std::u16string(u"open \"").append(utf8::utf8to16(manual_path)).append(u"\"");
    payload.command_runner.runCommand(open_command, false);

    // The dispatch switched the active context on success; on a failed open (deleted
    // manual) the active buffer keeps its name and the open error stays in the prompt.
    auto &context = m_context_manager.active();
    if (context.cursor.getName() != manual_path) {
        return std::nullopt;
    }

    if (heading == nullptr) {
        return std::nullopt;
    }

    // Jump to the section heading. The scroll is placed directly so the heading sits at
    // the top of the view (follow_indicator would leave it at the bottom edge); the
    // render clamps the value near the end of the file.
    const auto line_count = context.cursor.getLineCount();
    for (uint32_t line_index = 0; line_index < line_count; ++line_index) {
        if (context.cursor.getString(line_index).starts_with(heading)) {
            context.cursor.activateSelection(false);
            context.cursor.setPosition(line_index, 0);
            context.stick.index = context.cursor.getColumn();
            context.search.resetMatches();
            context.scroll.x = 0;
            context.scroll.y = static_cast<int64_t>(line_index) * context.theme.getLineHeight();
            context.wants_redraw = true;
            return std::nullopt;
        }
    }

    return std::u16string(u"Section not found: ").append(args[0]);
}
