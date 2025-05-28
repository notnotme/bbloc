#include "SaveFileCommand.h"

#include <filesystem>
#include <fstream>

#include <utf8.h>

#include "../core/CommandManager.h"


void SaveFileCommand::provideAutoComplete(const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    if (argumentIndex != 0) {
        // Only auto-complete the first argument (path)
        return;
    }

    CommandManager::getPathCompletions(input, false, itemCallback);
}

std::optional<std::u16string> SaveFileCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    // Check argument counts, keep the cursor name in a variable.
    const auto cursor_name = std::filesystem::path(payload.cursor.getName());
    if (cursor_name.empty() && (args.empty() || (args.size() >= 2 && args[1] != u"-f"))) {
        return u"Usage: save <filename> [-f]";
    }

    // Check if the file can be saved.
    const auto arg_filename = std::filesystem::path(args.empty() ? "" : utf8::utf16to8(args[0]));
    const auto file_to_save = arg_filename.empty() ? cursor_name : arg_filename;
    const auto file_exists = std::filesystem::exists(file_to_save);
    const auto is_regular_file = std::filesystem::is_regular_file(file_to_save);
    if (file_exists && !is_regular_file) {
        return std::u16string(u"Could not save ").append(args[0]).append(u".");
    }

    // CHeck if the file is overwritten by the operation
    if (cursor_name.filename() != file_to_save.filename()
        && file_exists
        && (args.size() == 1 || args[1] != u"-f")) {
        // Needs user feedback to be able to overwrite it
        payload.command_feedback = CommandFeedback {
            .prompt_message = u"File already exists, overwrite ? [y/N]:",
            // This reuses the same command, but with "-f" argument to force overwriting.
            .command_string = std::u16string(u"save ").append(args[0]).append(u" -f"),
            .completions_list = {u"n", u"y"},
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
    if(!ofs || !ofs.is_open()) {
        return std::u16string(u"Could not save ").append(args[0]).append(u".");
    }

    // Write all the text into the file.
    const auto line_count = payload.cursor.getLineCount();
    for (auto line = 0; line < line_count; ++line) {
        const auto string = payload.cursor.getString(line);
        ofs << utf8::utf16to8(string);
        if(line < line_count - 1) {
            // Append \n but at the end of the file.
            ofs << "\n";
        }
    }

    // Close file and set cursor name.
    ofs.close();
    payload.cursor.setName(file_to_save.string());

    // We always want to redraw, in case we run from a prompt confirmation.
    payload.wants_redraw = true;
    return std::nullopt;
}
