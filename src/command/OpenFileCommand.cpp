#include "OpenFileCommand.h"

#include <filesystem>
#include <fstream>

#include <utf8.h>

#include "../core/CommandManager.h"


void OpenFileCommand::provideAutoComplete(const int32_t argumentIndex, const std::string_view input, const AutoCompleteCallback<char> &itemCallback) const {
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

    const auto path = utf8::utf16to8(args[0]);
    const auto is_regular_file = std::filesystem::is_regular_file(path);
    auto ifs = std::ifstream(path, std::ios::in);
    if (!ifs || !ifs.is_open() || !is_regular_file) {
        // That file cannot be opened
        return std::u16string(u"Could not open ").append(args[0]).append(u".");
    }

    // Clear the cursor and read the file line by line
    const auto &edit_clear = payload.cursor.clear();
    payload.highlighter.edit(edit_clear);

    const auto file_extension = std::filesystem::path(path).extension().string();
    payload.highlighter.setMode(file_extension);

    auto line_count = 1u;
    auto line = std::string();
    auto all_line = std::u16string();
    while (getline(ifs, line)) {
        if (const auto &end_it = utf8::find_invalid(line.begin(), line.end()); end_it != line.end()) {
            // Invalid sequence: stop reading the file
            const auto utf16_line_count = utf8::utf8to16(std::to_string(line_count));
            return std::u16string(u"Invalid UTF-8 encoding detected at line ").append(utf16_line_count);
        }
        // Convert to utf16 then append to the cursor
        all_line.append(utf8::utf8to16(line));
        if (!ifs.eof() && !ifs.fail()) {
            // After the first insert, line ends with \n, but not the last
            all_line.append(u"\n");
        }

        ++line_count;
    }
    ifs.close();

    const auto &edit_insert = payload.cursor.insert(all_line);
    payload.highlighter.edit(edit_insert);

    payload.cursor.setName(path);
    payload.cursor.setPosition(0, 0);
    payload.follow_indicator = true;

    // In case the command is bound to a key, it will needs a redraw the views.
    // Otherwise, when the user type enter wants_redraw is set to true when processing the events.
    payload.wants_redraw = true;
    return std::nullopt;
}
