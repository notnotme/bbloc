#include "ExecCommand.h"

#include <filesystem>
#include <fstream>

#include "../core/CommandManager.h"


void ExecCommand::provideAutoComplete(const int32_t argumentIndex, const std::string_view input, const AutoCompleteCallback<char> &itemCallback) const {
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

    const auto path = utf8::utf16to8(args[0]);
    const auto is_regular_file = std::filesystem::is_regular_file(path);
    auto ifs = std::ifstream(path, std::ios::in);
    if (!ifs || !ifs.is_open() || !is_regular_file) {
        // That file cannot be opened
        return std::u16string(u"Could not open ").append(args[0]).append(u".");
    }

    auto command_list = std::vector<std::u16string>();
    auto line_count = 1;
    auto line = std::string();
    while (getline(ifs, line)) {
        if (const auto &end_it = utf8::find_invalid(line.begin(), line.end()); end_it != line.end()) {
            // Invalid sequence: stop the command list
            const auto utf16_line_count = utf8::utf8to16(std::to_string(line_count));
            return std::u16string(u"Invalid UTF-8 encoding detected at line ").append(utf16_line_count);
        }

        // Convert to utf16 then append to the cursor
        if (!line.starts_with("#")) {
            const auto u16string = utf8::utf8to16(line);
            command_list.emplace_back(u16string);
        }
        ++line_count;
    }
    ifs.close();

    for (const auto &command : command_list) {
        // fixme?: At this point, any feedback needed will interrupt the command list execution
        // fixme?: autoexec does not show in history
        // todo: take in account "#" as comment and don't execute the line
        payload.command_runner.runCommand(command, false);
    }

    return std::nullopt;
}
