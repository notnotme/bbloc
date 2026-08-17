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

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/CursorContextManager.h"
#include "../core/base/Command.h"
#include "../core/cvar/CVarInt.h"


/**
 * @brief Command for opening files in the text editor.
 *
 * This class implements the Command interface for opening files,
 * allowing users to load file content into the editor.
 * It handles file paths, with auto-completion support
 * for existing files in the filesystem.
 * A file loads into the active context when it is pristine (no name, empty buffer);
 * otherwise it opens in its own new context, which becomes the active one.
 * A file already open in another context is activated instead of loaded again,
 * so the same file never lives in two diverging buffers.
 * Opening a file larger than the open_size_limit CVar asks for confirmation first,
 * which the -f flag skips.
 */
class OpenFileCommand final : public Command<CursorContext> {
private:
    /** Reference to the manager owning the open cursor contexts. */
    CursorContextManager &m_context_manager;

    /** CVar holding the size in megabytes past which opening asks for confirmation; 0 disables the guard. */
    std::shared_ptr<CVarInt> m_open_size_limit;

    /**
     * @brief Finds the open context already holding the file at the given path.
     *
     * Names are stored as typed, so an exact string match is tried first; the paths
     * are then compared in weakly canonical form, so "./a.txt" and "a.txt" name the
     * same file. When canonicalization fails, only the exact spelling can match.
     *
     * @param path UTF-8 encoded path of the file to look up.
     * @return The index of the matching context, or std::nullopt when none holds it.
     */
    [[nodiscard]] std::optional<size_t> findOpenContext(const std::string &path) const;

    /**
     * @brief Reads the file at the given path and validates its encoding.
     *
     * The content is only written to outContent when the whole file is valid,
     * so a failed read never touches any buffer.
     *
     * @param path UTF-8 encoded path of the file to read.
     * @param outContent Receives the UTF-16 converted file content on success.
     * @return An error message on failure, std::nullopt on success.
     */
    [[nodiscard]] static std::optional<std::u16string> readFile(const std::string &path, std::u16string &outContent);

    /**
     * @brief Replaces the target context's buffer with the given content.
     *
     * Switches the highlight mode from the file extension, names the cursor after
     * the path, resets its position and scroll state, and discards the undo history.
     *
     * @param target The context receiving the file content.
     * @param path UTF-8 encoded path of the loaded file.
     * @param content UTF-16 converted file content.
     */
    static void loadInto(CursorContext &target, const std::string &path, std::u16string_view content);

public:
    /**
     * @brief Constructs an OpenFileCommand with a reference to the cursor context manager.
     *
     * @param contextManager Reference to the manager owning the open cursor contexts.
     * @param openSizeLimit CVar holding the confirmation threshold in megabytes; 0 disables it.
     */
    explicit OpenFileCommand(CursorContextManager &contextManager, std::shared_ptr<CVarInt> openSizeLimit);

    /**
     * @brief Provides auto-completion suggestions for file paths.
     *
     * This command auto-completes argument 0 which is the file path,
     * and argument 1 which can only be the -f flag.
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
     * An optional second argument, -f, skips the large-file confirmation.
     *
     * @param payload The cursor context that will be updated with the new file content.
     * @param args Command arguments specifying the file path to open.
     * @return An optional message indicating the result of the operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, std::span<const std::u16string_view> args) override;
};


#endif //OPEN_FILE_COMMAND_H
