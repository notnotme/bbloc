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
#include "SaveFileCommand.h"

#include <filesystem>
#include <fstream>
#include <system_error>

#include <utf8.h>

#include "../core/CommandManager.h"


/** @brief Writes a UTF-8 payload to a path, truncating it; returns false when anything went wrong. */
static bool writeFile(const std::filesystem::path &path, const std::string &content) {
    auto ofs = std::ofstream(path, std::ios::out);
    if (!ofs) {
        return false;
    }

    // Write everything, then close explicitly so a failure flushing the last block is reported here.
    ofs << content;
    ofs.close();
    return !ofs.fail();
}

void SaveFileCommand::provideAutoComplete(const std::span<const std::u16string_view> previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) previousArgs;
    if (argumentIndex == 0) {
        // The first argument is the path
        CommandManager::getPathCompletions(input, false, itemCallback);
    } else if (argumentIndex == 1) {
        // The second argument can only be the overwrite flag
        constexpr auto force_flag = std::u16string_view(u"-f");
        if (force_flag.starts_with(input)) {
            itemCallback(force_flag);
        }
    }
}

std::optional<std::u16string> SaveFileCommand::run(CursorContext &payload, const std::span<const std::u16string_view> args) {
    // Check argument counts, keep the cursor name in a variable.
    const auto cursor_name = std::filesystem::path(payload.cursor.getName());
    if (cursor_name.empty() && args.empty() && !payload.from_prompt) {
        // From the prompt the filename is mandatory; from the editor, ask for it interactively.
        payload.command_feedback = requestPathArgument(u"save ", u"save", payload.command_runner,
            [](const std::u16string_view input, const AutoCompleteCallback &itemCallback) {
                CommandManager::getPathCompletions(input, false, itemCallback);
            });

        return std::nullopt;
    }

    if (cursor_name.empty() && (args.empty() || (args.size() >= 2 && args[1] != u"-f"))) {
        return u"Usage: save <filename> [-f]";
    }

    // Read the overwrite flag once: the checks below must not index an argument list that may be empty.
    const auto force_overwrite = args.size() >= 2 && args[1] == u"-f";

    // Check if the file can be saved.
    const auto arg_filename = std::filesystem::path(args.empty() ? "" : utf8::utf16to8(args[0]));
    const auto file_to_save = arg_filename.empty() ? cursor_name : arg_filename;
    const auto file_to_save_utf16 = utf8::utf8to16(file_to_save.string());
    auto error_code = std::error_code{};
    const auto file_exists = std::filesystem::exists(file_to_save, error_code);
    const auto is_regular_file = std::filesystem::is_regular_file(file_to_save, error_code);
    if (file_exists && !is_regular_file) {
        return std::u16string(u"Could not save ").append(file_to_save_utf16).append(u".");
    }

    // Compare full canonical paths, so "./a.txt" and "a.txt" name the same file.
    // On resolution failure, treat the paths as different: prompting is the safe default.
    auto cursor_name_error = std::error_code{};
    auto file_to_save_error = std::error_code{};
    const auto canonical_cursor_name = std::filesystem::weakly_canonical(cursor_name, cursor_name_error);
    const auto canonical_file_to_save = std::filesystem::weakly_canonical(file_to_save, file_to_save_error);
    const auto is_same_file = !cursor_name_error && !file_to_save_error && canonical_cursor_name == canonical_file_to_save;

    // Check if the file is overwritten by the operation
    if (!is_same_file
        && file_exists
        && !force_overwrite) {
        // Re-quote the path so the rerun tokenizes it back to one argument.
        const auto quoted_filename = quoteArgument(file_to_save_utf16);

        // Needs user feedback to be able to overwrite it
        payload.command_feedback = CommandFeedback {
            .prompt_message = u"File already exists, overwrite ? [y/N]: ",
            // This reuses the same command, but with "-f" argument to force overwriting.
            .command_string = std::u16string(u"save ").append(quoted_filename).append(u" -f"),
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

    // Encode the whole document before touching the filesystem: an encoding failure must not
    // have destroyed the previous content already.
    auto utf8_text = std::string{};
    try {
        utf8_text = utf8::utf16to8(payload.cursor.getText());
    } catch (const utf8::exception &) {
        return std::u16string(u"Could not encode ").append(file_to_save_utf16).append(u" as UTF-8.");
    }

    // Preferred path: write a sibling temporary file, then rename it over the destination. Opening
    // the destination directly truncates it, so a failed write would leave nothing behind.
    auto temporary_file = file_to_save;
    temporary_file += ".tmp";

    auto saved_atomically = false;
    if (writeFile(temporary_file, utf8_text)) {
        // The temporary file was created with the default mode: carry the destination's own
        // permissions over, so overwriting does not widen them. A failure here is not fatal.
        if (file_exists) {
            auto permission_error = std::error_code{};
            if (const auto status = std::filesystem::status(file_to_save, permission_error); !permission_error) {
                std::filesystem::permissions(temporary_file, status.permissions(), permission_error);
            }
        }

        // Move the temporary file into place; on the same filesystem this replaces the destination
        // in one step, so it is never observed half-written.
        auto rename_error = std::error_code{};
        std::filesystem::rename(temporary_file, file_to_save, rename_error);
        saved_atomically = !rename_error;
    }

    if (!saved_atomically) {
        // Atomicity is not always available: a writable file inside a read-only directory rejects
        // the sibling temporary file, and some filesystems (the Switch SD card) refuse to rename
        // onto an existing path. Drop whatever the attempt left behind and write in place instead.
        auto remove_error = std::error_code{};
        std::filesystem::remove(temporary_file, remove_error);

        if (!writeFile(file_to_save, utf8_text)) {
            return std::u16string(u"Could not save ").append(file_to_save_utf16).append(u".");
        }
    }

    // The file is written: set the cursor name, and the buffer content now matches the disk.
    payload.cursor.setName(file_to_save.string());
    payload.cursor.setModified(false);

    // We always want to redraw, in case we run from a prompt confirmation.
    payload.wants_redraw = true;
    return std::nullopt;
}
