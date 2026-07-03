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
#ifndef REDO_COMMAND_H
#define REDO_COMMAND_H

#include <string>
#include <vector>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"


/**
 * @brief Command for re-applying the last undone text modification in the editor.
 *
 * This class implements the Command interface for redo operations,
 * restoring the buffer and cursor to their state before the last undo.
 */
class RedoCommand final : public Command<CursorContext> {
public:
    /** @brief Constructs a RedoCommand with default initialization. */
    explicit RedoCommand() = default;

    /**
      * @brief Provides auto-completion suggestions for command arguments.
      *
      * This command does not auto-complete.
      *
      * @param previousArgs The arguments typed before the one being completed, excluding the command name.
      * @param argumentIndex The index of the argument currently being completed.
      * @param input The current partial input from the user for this argument.
      * @param itemCallback A callback to be invoked with each completion suggestion.
      */
    void provideAutoComplete(const std::vector<std::u16string_view> &previousArgs, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) const override;

    /**
     * @brief Executes the redo operation.
     *
     * Restores the last undone buffer snapshot and forwards the resulting edit to the highlighter.
     * This command expects 0 argument (empty Vector).
     *
     * @param payload The cursor context that will be modified by the redo operation.
     * @param args Command arguments (typically unused for redo operations).
     * @return An optional message indicating the result of the operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;
};


#endif //REDO_COMMAND_H
