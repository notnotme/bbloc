#include "OpenFileCommand.h"

#include <filesystem>
#include <fstream>

#include <utf8.h>

#include "../core/CommandManager.h"


void OpenFileCommand::provideAutoComplete(const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
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
    const auto is_regular_file = std::filesystem::is_regular_file(path);
    auto ifs = std::ifstream(path, std::ios::in);
    if (!ifs || !ifs.is_open() || !is_regular_file) {
        // That file cannot be opened
        return std::u16string(u"Could not open ").append(args[0]).append(u".");
    }

    // Clear the cursor, find the file extension to set the highlight mode, and read the file line by line.
    const auto &edit_clear = payload.cursor.clear();
    payload.highlighter.edit(edit_clear);

    const auto file_extension = std::filesystem::path(path).extension().string();
    payload.highlighter.setMode(file_extension);

    // Start to count the lines from 1
    auto line_count = 1u;
    // Stores temporary line and the whole text.
    auto line = std::string();
    auto all_line = std::u16string();
    while (getline(ifs, line)) {
        if (const auto &end_it = utf8::find_invalid(line.begin(), line.end()); end_it != line.end()) {
            // Invalid sequence: stop reading the file
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

    // Insert all text at once.
    const auto &edit_insert = payload.cursor.insert(all_line);
    payload.highlighter.edit(edit_insert);

    // Set cursor name and reset position.
    payload.cursor.setName(path);
    payload.cursor.setPosition(0, 0);
    payload.follow_indicator = true;

    // In case the command is bound to a key, it will eventually needs a redraw the views.
    payload.wants_redraw = true;
    return std::nullopt;
}
