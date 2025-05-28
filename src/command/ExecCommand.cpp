#include "ExecCommand.h"

#include <filesystem>
#include <fstream>

#include <utf8.h>

#include "../core/CommandManager.h"


void ExecCommand::provideAutoComplete(const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    if (argumentIndex != 0) {
        // Only auto-complete the first argument (path)
        return;
    }

    CommandManager::getPathCompletions(input, false, itemCallback);
}

std::optional<std::u16string> ExecCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    if (args.size() != 1) {
        return u"Usage: exec <filename>";
    }

    // Get the path of the file and tries to open the file at this location
    const auto path = utf8::utf16to8(args[0]);
    const auto is_regular_file = std::filesystem::is_regular_file(path);
    auto ifs = std::ifstream(path, std::ios::in);
    if (!ifs || !ifs.is_open() || !is_regular_file) {
        // That file cannot be opened
        return std::u16string(u"Could not open ").append(args[0]).append(u".");
    }

    // This will store the command list to run and the current line of the file that we are reading.
    auto command_list = std::vector<std::u16string>();
    auto line = std::string();

    auto line_count = 1;
    while (getline(ifs, line)) {
        if (const auto &end_it = utf8::find_invalid(line.begin(), line.end()); end_it != line.end()) {
            // Invalid sequence: stop the command list
            const auto line_count_str = std::to_string(line_count);
            const auto utf16_line_count_str = utf8::utf8to16(line_count_str);
            return std::u16string(u"Invalid UTF-8 encoding detected at line ").append(utf16_line_count_str);
        }

        // Convert to utf16 then append to the cursor
        if (!line.starts_with("#")) {
            // If the line starts with "#", this is a comment, otherwise this is a command.
            const auto u16string = utf8::utf8to16(line);
            command_list.emplace_back(u16string);
        }
        ++line_count;
    }
    ifs.close();

    for (const auto &command : command_list) {
        // fixme?: At this point, any feedback needed will interrupt the command list execution
        // fixme!: This is not well tested at all.
        payload.command_runner.runCommand(command, false);
    }

    return std::nullopt;
}
