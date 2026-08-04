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
#include "QuitCommand.h"

#include <SDL_events.h>


QuitCommand::QuitCommand(CursorContextManager &contextManager)
    : m_context_manager(contextManager) {}

void QuitCommand::provideAutoComplete(const std::span<const std::u16string_view> previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) previousArgs;
    (void) argumentIndex;
    (void) input;
    (void) itemCallback;
    // No-op
}

std::optional<std::u16string> QuitCommand::run(CursorContext &payload, const std::span<const std::u16string_view> args) {
    // The only accepted argument is the "-f" flag skipping the unsaved-changes prompt.
    const auto is_forced = args.size() == 1 && args[0] == u"-f";
    if (!args.empty() && !is_forced) {
        return u"Usage: quit [-f]";
    }

    // A single confirmation covers every open buffer holding unsaved changes.
    if (!is_forced) {
        auto any_modified = false;
        for (size_t index = 0; index < m_context_manager.getCount(); ++index) {
            if (m_context_manager.get(index).cursor.isModified()) {
                any_modified = true;
                break;
            }
        }

        if (any_modified) {
            payload.command_feedback = CommandFeedback {
                .prompt_message = u"Unsaved changes, quit anyway ? [y/N]: ",
                // This reuses the same command, but with "-f" argument to skip this prompt.
                .command_string = u"quit -f",
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
    }

    // Push an SDL_QUIT event, and the event loop will catch it.
    SDL_Event event { .type = SDL_QUIT };
    SDL_PushEvent(&event);
    return std::nullopt;
}
