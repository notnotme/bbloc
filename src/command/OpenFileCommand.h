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
#ifndef OPEN_FILE_COMMAND_H
#define OPEN_FILE_COMMAND_H

#include <span>
#include <string>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"


/**
 * @brief Command for opening files in the text editor.
 *
 * This class implements the Command interface for opening files,
 * allowing users to load file content into the editor.
 * It handles file paths, with auto-completion support
 * for existing files in the filesystem.
 */
class OpenFileCommand final : public Command<CursorContext> {
public:
    /** @brief Constructs an OpenFileCommand with default initialization. */
    explicit OpenFileCommand() = default;

    /**
     * @brief Provides auto-completion suggestions for file paths.
     *
     * This command auto-completes argument 0 which is the file path
     *
     * @param previousArgs The arguments typed before the one being completed, excluding the command name.
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideAutoComplete(std::span<const std::u16string_view> previousArgs, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) const override;

    /**
     * @brief Executes the file opening operation.
     *
     * Opens the specified file and loads its content into the editor.
     * Expect 1 argument which is the file path. The file path must be "quoted" if it contains blank characters (spaces).
     *
     * @param payload The cursor context that will be updated with the new file content.
     * @param args Command arguments specifying the file path to open.
     * @return An optional message indicating the result of the operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, std::span<const std::u16string_view> args) override;
};


#endif //OPEN_FILE_COMMAND_H
