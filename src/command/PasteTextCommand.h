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
#ifndef PASTE_TEXT_COMMAND_H
#define PASTE_TEXT_COMMAND_H

#include <span>
#include <string>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"


/**
 * @brief Command for pasting text from the clipboard into the editor.
 *
 * This class implements the Command interface for pasting text operations,
 * allowing users to insert clipboard content at the current cursor position.
 */
class PasteTextCommand final : public Command<CursorContext> {
public:
    /** @brief Constructs a PasteTextCommand with default initialization. */
    explicit PasteTextCommand() = default;

    /**
      * @brief Provides auto-completion suggestions for command arguments.
      *
      * THos command does not auto-complete.
      *
      * @param previousArgs The arguments typed before the one being completed, excluding the command name.
      * @param argumentIndex The index of the argument currently being completed.
      * @param input The current partial input from the user for this argument.
      * @param itemCallback A callback to be invoked with each completion suggestion.
      */
    void provideAutoComplete(std::span<const std::u16string_view> previousArgs, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) const override;

    /**
     * @brief Executes the paste operation.
     *
     * Retrieves text from the system clipboard and inserts it at the
     * current cursor position in the document.
     * This command expects 0 argument (empty Vector).
     *
     * @param payload The cursor context that will be modified by the paste operation.
     * @param args Command arguments (typically unused for paste operations).
     * @return An optional message indicating the result of the operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, std::span<const std::u16string_view> args) override;
};


#endif //PASTE_TEXT_COMMAND_H
