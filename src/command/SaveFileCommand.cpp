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


void SaveFileCommand::provideAutoComplete(const std::span<const std::u16string_view> previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) previousArgs;
    if (argumentIndex == 0) {
        // The first argument is the path
        CommandManager::getPathCompletions(input, false, itemCallback);
    } else if (argumentIndex == 1) {
        // The second argument can only be the overwrite flag
        constexpr auto FORCE_FLAG = std::u16string_view(u"-f");
        if (FORCE_FLAG.starts_with(input)) {
            itemCallback(FORCE_FLAG);
        }
    }
}

std::optional<std::u16string> SaveFileCommand::run(CursorContext &payload, const std::span<const std::u16string_view> args) {
    // Check argument counts, keep the cursor name in a variable.
    const auto cursor_name = std::filesystem::path(payload.cursor.getName());
    if (cursor_name.empty() && args.empty() && !payload.from_prompt) {
        // From the prompt the filename is mandatory; from the editor, ask for it interactively.
        payload.command_feedback = CommandFeedback {
            .prompt_message = u"save ",
            .command_string = u"save",
            .on_complete_callback = [](const std::u16string_view input, const AutoCompleteCallback &itemCallback) {
                CommandManager::getPathCompletions(input, false, itemCallback);
            },
            .on_validate_callback = [&](const std::u16string_view input, const std::u16string_view command) -> std::optional<std::u16string> {
                // Re-quote a path containing spaces so the rerun tokenizes it back to one argument.
                // An answer already holding a quote is passed verbatim: the tokenizer handles it.
                auto quoted_filename = std::u16string(input);
                if (quoted_filename.find(u' ') != std::u16string::npos && quoted_filename.find(u'"') == std::u16string::npos) {
                    quoted_filename = std::u16string(u"\"").append(quoted_filename).append(u"\"");
                }

                payload.command_runner.runCommand(std::u16string(command).append(u" ").append(quoted_filename), true);
                return std::nullopt;
            }
        };

        return std::nullopt;
    }

    if (cursor_name.empty() && (args.empty() || (args.size() >= 2 && args[1] != u"-f"))) {
        return u"Usage: save <filename> [-f]";
    }

    // Check if the file can be saved.
    const auto arg_filename = std::filesystem::path(args.empty() ? "" : utf8::utf16to8(args[0]));
    const auto file_to_save = arg_filename.empty() ? cursor_name : arg_filename;
    const auto file_to_save_utf16 = utf8::utf8to16(file_to_save.string());
    auto error_code = std::error_code();
    const auto file_exists = std::filesystem::exists(file_to_save, error_code);
    const auto is_regular_file = std::filesystem::is_regular_file(file_to_save, error_code);
    if (file_exists && !is_regular_file) {
        return std::u16string(u"Could not save ").append(file_to_save_utf16).append(u".");
    }

    // Compare full canonical paths, so "./a.txt" and "a.txt" name the same file.
    // On resolution failure, treat the paths as different: prompting is the safe default.
    auto cursor_name_error = std::error_code();
    auto file_to_save_error = std::error_code();
    const auto canonical_cursor_name = std::filesystem::weakly_canonical(cursor_name, cursor_name_error);
    const auto canonical_file_to_save = std::filesystem::weakly_canonical(file_to_save, file_to_save_error);
    const auto is_same_file = !cursor_name_error && !file_to_save_error && canonical_cursor_name == canonical_file_to_save;

    // Check if the file is overwritten by the operation
    if (!is_same_file
        && file_exists
        && (args.size() == 1 || args[1] != u"-f")) {
        // Re-quote a path containing spaces so the rerun tokenizes it back to one argument.
        auto quoted_filename = file_to_save_utf16;
        if (quoted_filename.find(u' ') != std::u16string::npos) {
            quoted_filename = std::u16string(u"\"").append(quoted_filename).append(u"\"");
        }

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

    // Prepare to output all the text.
    auto ofs = std::ofstream(file_to_save, std::ios::out);
    if (!ofs) {
        return std::u16string(u"Could not save ").append(file_to_save_utf16).append(u".");
    }

    // Write all the text into the file.
    try {
        ofs << utf8::utf16to8(payload.cursor.getText());
    } catch (const utf8::exception &) {
        return std::u16string(u"Could not encode ").append(file_to_save_utf16).append(u" as UTF-8.");
    }

    // Close file, report a failed write, and set cursor name.
    ofs.close();
    if (ofs.fail()) {
        return std::u16string(u"Could not save ").append(file_to_save_utf16).append(u".");
    }
    payload.cursor.setName(file_to_save.string());

    // We always want to redraw, in case we run from a prompt confirmation.
    payload.wants_redraw = true;
    return std::nullopt;
}
