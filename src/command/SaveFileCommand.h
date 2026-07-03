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
#ifndef SAVE_FILE_COMMAND_H
#define SAVE_FILE_COMMAND_H

#include <string>
#include <vector>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"


/**
 * @brief Command for saving the current file.
 *
 * This class implements the Command interface for saving the active document
 * to disk. It handles the case where the file already exists and will trigger feedback
 * to be able to erase the existing file.
 */
class SaveFileCommand final : public Command<CursorContext> {
public:
    /** @brief Constructs a SaveFileCommand with default initialization. */
    explicit SaveFileCommand() = default;

    /**
     * @brief Provides auto-completion suggestions for command arguments.
     *
     * Implements the Command interface method for auto-completion.
     * This command auto-completes argument 0 which is the file path, and argument 1 which is the "-f" flag.
     *
     * @param previousArgs The arguments typed before the one being completed, excluding the command name.
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideAutoComplete(const std::vector<std::u16string_view> &previousArgs, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) const override;

    /**
     * @brief Executes the file save operation.
     *
     * Saves the current document to disk. If no arguments are provided, saves to the
     * current file using the cursor's name. Otherwise, will try to save the file corresponding to argument 0 and will
     * ask the user for confirmation (using feedback) before overwriting the existing file.
     * Expect 1 or 2 arguments:
     * - The file path
     * - "-f" If used, does not ask the user for confirmation before overwriting files.
     *
     * @param payload The cursor context containing the document to be saved.
     * @param args Command arguments, with an optional first argument specifying the save path.
     * @return An optional message indicating the result of the operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;
};


#endif //SAVE_FILE_COMMAND_H
