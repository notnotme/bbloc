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
#ifndef CUT_TEXT_COMMAND_H
#define CUT_TEXT_COMMAND_H

#include <span>
#include <string>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"


/**
 * @brief Command for cutting text to the clipboard in the text editor.
 *
 * This class implements the Command interface for cutting selected text,
 * which copies the text to the system clipboard and then removes it from
 * the cursor text buffer.
 */
class CutTextCommand final : public Command<CursorContext> {
public:
    /** @brief Constructs a CutTextCommand with default initialization. */
    explicit CutTextCommand() = default;

    /**
     * @brief Provides auto-completion suggestions for command arguments.
     *
     * This command does not expect any completion.
     *
     * @param previousArgs The arguments typed before the one being completed, excluding the command name.
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideAutoComplete(std::span<const std::u16string_view> previousArgs, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) const override;

    /**
     * @brief Executes the cut operation.
     *
     * Copies the currently selected text to the system clipboard and then
     * removes it from the cursor.
     * Expect 0 arguments (empty vector).
     *
     * @param payload The cursor context containing the current selection and document state.
     * @param args Command arguments that may modify the cut behavior.
     * @return An optional message indicating the result of the cut operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, std::span<const std::u16string_view> args) override;
};


#endif //CUT_TEXT_COMMAND_H
