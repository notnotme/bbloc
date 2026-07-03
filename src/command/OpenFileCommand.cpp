#include "OpenFileCommand.h"

#include <filesystem>
#include <fstream>
#include <system_error>

#include <utf8.h>

#include "../core/CommandManager.h"


void OpenFileCommand::provideAutoComplete(const std::vector<std::u16string_view> &previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) previousArgs;
    if (argumentIndex != 0) {
        // Only auto-complete the first argument (path)
        return;
    }

    CommandManager::getPathCompletions(input, false, itemCallback);
}

std::optional<std::u16string> OpenFileCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    if (args.size() != 1) {
        return u"Usage: open <filename>";
    }

    // Get the path of the file and tries to open the file at this location
    const auto path = utf8::utf16to8(args[0]);
    auto error_code = std::error_code();
    const auto is_regular_file = std::filesystem::is_regular_file(path, error_code);
    auto ifs = std::ifstream(path, std::ios::in);
    if (!ifs || !is_regular_file) {
        // That file cannot be opened
        return std::u16string(u"Could not open ").append(args[0]).append(u".");
    }

    // Start to count the lines from 1
    auto line_count = 1u;
    // Stores temporary line and the whole text.
    auto line = std::string();
    auto all_line = std::u16string();
    while (getline(ifs, line)) {
        if (!line.empty() && line.back() == '\r') {
            // Strip the carriage return of CRLF line endings
            line.pop_back();
        }

        if (const auto &end_it = utf8::find_invalid(line.begin(), line.end()); end_it != line.end()) {
            // Invalid sequence: stop reading the file, before the buffer is touched
            const auto line_count_str = std::to_string(line_count);
            const auto utf16_line_count_str = utf8::utf8to16(line_count_str);
            return std::u16string(u"Invalid UTF-8 encoding detected at line ").append(utf16_line_count_str);
        }
        // Convert to utf16 then append to the cursor
        const auto utf16_line = utf8::utf8to16(line);
        all_line.append(utf16_line);
        if (!ifs.eof() && !ifs.fail()) {
            // After the first insert, line ends with \n, but not the last
            all_line.append(u"\n");
        }

        ++line_count;
    }
    ifs.close();

    // The whole content is validated: it is now safe to replace the buffer and switch the highlight mode.
    const auto &edit_clear = payload.cursor.clear();
    payload.highlighter.edit(edit_clear);

    const auto file_extension = std::filesystem::path(path).extension().string();
    payload.highlighter.setMode(file_extension);

    // Insert all text at once.
    const auto &edit_insert = payload.cursor.insert(all_line);
    payload.highlighter.edit(edit_insert);

    // Set cursor name, reset position and discard the undo history of the previous buffer.
    payload.cursor.setName(path);
    payload.cursor.setPosition(0, 0);
    payload.cursor.clearHistory();
    payload.follow_indicator = true;
    payload.stick_to_column = false;
    payload.stick_column_index = 0;

    // In case the command is bound to a key, it will eventually needs a redraw the views.
    payload.wants_redraw = true;
    return std::nullopt;
}
