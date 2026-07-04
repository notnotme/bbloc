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
#ifndef COPY_TEXT_COMMAND_H
#define COPY_TEXT_COMMAND_H

#include <string>
#include <vector>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"


/**
 * @brief Command for copying text from the text editor to the clipboard.
 *
 * This class implements the Command interface for copying selected text
 * to the system clipboard.
 */
class CopyTextCommand final : public Command<CursorContext> {
public:
    /** @brief Constructs a CopyTextCommand with default initialization. */
    explicit CopyTextCommand() = default;

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
    void provideAutoComplete(const std::vector<std::u16string_view> &previousArgs, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) const override;

    /**
     * @brief Executes the copy operation.
     *
     * Copies the currently selected text to the system clipboard.
     * Expect 0 arguments (empty vector).
     *
     * @param payload The cursor context containing the current selection and document state.
     * @param args Command arguments that may modify the copy behavior.
     * @return An optional message indicating the result of the copy operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;

    /**
     * @brief Copies the currently selected text to the system clipboard.
     *
     * Joins the selected lines with line feeds and hands the UTF-8 text to SDL.
     * Shared with CutTextCommand, which erases the selection afterwards.
     *
     * @param payload The cursor context containing the current selection.
     * @return An optional message describing why the copy failed, or std::nullopt on success.
     */
    [[nodiscard]] static std::optional<std::u16string> copySelectionToClipboard(const CursorContext &payload);
};


#endif //COPY_TEXT_COMMAND_H
