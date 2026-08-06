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
#ifndef HELP_COMMAND_H
#define HELP_COMMAND_H

#include <array>
#include <span>
#include <string>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/base/Command.h"
#include "../core/CursorContext.h"
#include "../core/CursorContextManager.h"


/**
 * @brief Command opening the user manual, optionally jumping to a section.
 *
 * "help" opens the ASCII manual (romfs/manual.txt, resolved through Platform::assetPath)
 * by dispatching the open command, so an already open manual buffer is switched to
 * instead of reloaded. "help <section>" then jumps to the matching "=== <Section>"
 * heading, scrolled to the top of the view; the section list is static and must stay in
 * sync with the manual's headings (docs/manual.md mirrors the content for GitHub).
 */
class HelpCommand final : public Command<CursorContext> {
private:
    /** @brief One jump target: the argument name and the manual heading it matches. */
    struct Section {
        const char16_t *argument; ///< Section name as typed by the user.
        const char16_t *heading;  ///< Exact heading line in the manual.
    };

    /** Jump targets, mirroring the manual's "===" headings. */
    static const std::array<Section, 6> SECTIONS;

    /** Context manager queried for the active context after the open dispatch switches it. */
    CursorContextManager &m_context_manager;

public:
    /**
     * @brief Constructs a HelpCommand.
     *
     * @param contextManager Reference to the cursor context manager.
     */
    explicit HelpCommand(CursorContextManager &contextManager);

    /**
     * @brief Provides auto-completion suggestions for the section argument.
     *
     * @param previousArgs The arguments typed before the one being completed.
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideAutoComplete(std::span<const std::u16string_view> previousArgs, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) const override;

    /**
     * @brief Executes the help command.
     *
     * @param payload The cursor context active when the command runs.
     * @param args Zero or one argument: the section to jump to.
     * @return An error message on an unknown section or a failed open; std::nullopt otherwise.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, std::span<const std::u16string_view> args) override;
};


#endif //HELP_COMMAND_H
