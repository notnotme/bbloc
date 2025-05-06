#include "CommandManager.h"

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <utf8.h>
#include <ranges>
#include <vector>


void CommandManager::create() {
    registerCVarCommand();
    registerExecCommand();
}

void CommandManager::destroy() {
    m_commands.clear();
    m_cvars.clear();
    m_command_feedback.reset();
}

void CommandManager::registerCommand(const std::string_view name, const CommandCallback& callback, const CompletionCallback& completionCallback) {
    const auto c_string = name.data();
    if (m_commands.contains(c_string)) {
        throw std::runtime_error(std::string("Command already registered: ").append(name));
    }

    const auto& [new_entry, success] = m_commands.insert({c_string, { .func = callback, .completion_func = completionCallback}});
    if (!success) {
        throw std::runtime_error(std::string("Unable to register command: ").append(name));
    }
}

void CommandManager::registerCvar(std::string_view name, std::shared_ptr<CVar> cvar, const CVarCallback& callback) {
    const auto c_string = name.data();
    if (m_cvars.contains(c_string)) {
        throw std::runtime_error(std::string("CVar already registered: ").append(name));
    }

    const auto& [new_entry, success] = m_cvars.insert({c_string, { .cvar = std::move(cvar), .callback = callback}});
    if (!success) {
        throw std::runtime_error(std::string("Unable to register  CVar: ").append(name));
    }
}

std::optional<std::u16string> CommandManager::execute(Cursor& cursor, const std::u16string_view input) {
    const auto tokens = tokenize(input);
    if (tokens.empty()) {
        // Nothing to process
        return std::nullopt;
    }

    if (m_command_feedback.has_value()) {
        // Before executing the command, check if we have a feedback to run
        const auto feedback = m_command_feedback.value();
        if (tokens.size() == 1) {
            m_command_feedback.reset();
            return feedback.on_validate_callback(tokens[0], feedback.command);
        }

        // Reset the prompt then
        return std::nullopt;
    }

    const auto command = utf8::utf16to8(tokens[0]);
    if (const auto cmd = m_commands.find(command); cmd != m_commands.end()) {
        // Skip the first item in the tokens, as it is the command name and we don't need it
        return cmd->second.func(cursor, {tokens.begin() + 1, tokens.end()});
    }

    return std::u16string(u"Unknown command: ").append(tokens[0]);
}

void CommandManager::setCommandFeedback(std::u16string_view prompt, std::u16string_view command, const std::vector<std::u16string_view>& completions, const FeedbackCallback& callback) {
    m_command_feedback = {
        .prompt = prompt.data(),
        .command = command.data(),
        .completions = { completions.begin(), completions.end() },
        .on_validate_callback = callback
    };
}

std::optional<std::u16string_view> CommandManager::getCommandFeedback() const {
    if (m_command_feedback.has_value()) {
        return m_command_feedback->prompt;
    }
    return std::nullopt;
}

void CommandManager::clearCommandFeedback() {
    m_command_feedback.reset();
}

void CommandManager::getFeedbackCompletion(const ItemCallback<char16_t>& itemCallback) const {
    if (m_command_feedback.has_value()) {
        for (const auto& completion : m_command_feedback->completions) {
            itemCallback(completion);
        }
    }
}

void CommandManager::getCommandCompletions(const std::string_view input, const ItemCallback<char>& itemCallback) {
    const auto input_is_empty = input.empty();
    for (const auto& name : std::views::keys(m_commands)) {
        if (name.starts_with(input) || input_is_empty) {
            // If input is empty, push everything to the list of possibilities
            itemCallback(name);
        }
    }
}

void CommandManager::getCVarCompletions(const std::string_view input, const ItemCallback<char>& itemCallback) {
    const auto input_is_empty = input.empty();
    for (const auto& name : std::views::keys(m_cvars)) {
        if (name.starts_with(input) || input_is_empty) {
            // If input is empty, push everything to the list of possibilities
            itemCallback(name);
        }
    }
}

void CommandManager::getArgumentsCompletion(const std::string_view command, const int32_t argumentIndex, const std::string_view input, const ItemCallback<char>& itemCallback) {
    if (const auto cmd = m_commands.find(command.data()); cmd != m_commands.end()) {
        if (cmd->second.completion_func != nullptr) {
            cmd->second.completion_func(argumentIndex, input, itemCallback);
        }
    }
}

void CommandManager::getPathCompletions(const std::string_view input, const bool foldersOnly, const ItemCallback<char>& itemCallback) {
    const auto path_input = std::filesystem::path(input);
    const auto directory = path_input.has_parent_path() ? path_input.parent_path() : ".";
    const auto input_path = path_input.filename().string();
    if (std::filesystem::exists(directory) && std::filesystem::is_directory(directory)) {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            const auto& path = entry.path();
            const auto& complete_path = path.string();
            const auto& filename = path.filename().string();
            if (filename.starts_with(input_path)) {
                if (entry.is_directory() || (!foldersOnly && entry.is_regular_file())) {
                    itemCallback(complete_path);
                }
            }
        }
    }
}

std::vector<std::u16string_view> CommandManager::tokenize(const std::u16string_view input) {
    std::vector<std::u16string_view> tokens;
    auto start = 0;
    auto index = 0;
    while (index < input.length()) {
        constexpr auto SPACE_DELIMITER = U' ';
        constexpr auto QUOTE_DELIMITER = U'"';

        // Skip blank spaces
        if (input[index] == SPACE_DELIMITER) {
            ++index;
            continue;
        }

        start = index;
        if (input[index] == QUOTE_DELIMITER) {
            // skip opening quote
            ++start;
            ++index;
            while (index < input.length() && input[index] != QUOTE_DELIMITER) {
                ++index;
            }

            if (index < input.length()) {
                tokens.emplace_back(input.substr(start, index - start));
                // skip closing quote
                ++index;
            } else {
                // Unterminated quote, take until the end
                tokens.emplace_back(input.substr(start));
                break;
            }
        } else {
            // Unquoted word
            while (index < input.size() && input[index] != SPACE_DELIMITER) {
                ++index;
            }
            tokens.emplace_back(input.substr(start, index - start));
        }
    }

    return tokens;
}

void CommandManager::registerCVarCommand() {
    // Register a "cvar" command that prints or set a CVar value
    // This will also call any callback registered with the said CVar
    registerCommand("cvar",
        [&](const Cursor& cursor, const std::vector<std::u16string_view>& args) -> std::optional<std::u16string> {
            (void) cursor;
            if (args.empty()) {
                return u"Usage: cvar <name> [value1] [value2] ...";
            }

            const auto cvar_name = utf8::utf16to8(args[0]);
            if (!m_cvars.contains(cvar_name)) {
                return std::u16string(u"Unknown cvar: ").append(args[0]);
            }

            const auto& cvar_entry = m_cvars[cvar_name];
            auto* cvar = cvar_entry.cvar.get();

            if (args.size() == 1) {
                // Print the value of this cvar
                return std::u16string(args[0]).append(u": ").append(cvar->getStringValue());
            }

            // Se the value of this cvar if not read-only
            if (cvar->isReadOnly()) {
                return std::u16string(u"CVar is read-only: ").append(args[0]);
            }

            if (const auto error = cvar->setValueFromStrings({args.begin() + 1, args.end()})) {
                // Something wrong happened
                return std::u16string(args[0]).append(u": ").append(error.value());
            }

            // Eventually invoke the associated callback
            if (const auto callback = cvar_entry.callback) {
                callback();
            }

            return std::nullopt;
        },
        [&](const int32_t argumentIndex, const std::string_view input, const ItemCallback<char>& itemCallback) {
            if (argumentIndex > 0) {
                // Only auto-complete the names of CVars
                return;
            }
            getCVarCompletions(input, itemCallback);
        });
}

void CommandManager::registerExecCommand() {
    // Register the exec command, that read a text file on disk and execute each line as command
    registerCommand("exec",
        [&](Cursor& cursor, const std::vector<std::u16string_view>& args) -> std::optional<std::u16string> {
            (void) cursor;
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
                if (const auto end_it = utf8::find_invalid(line.begin(), line.end()); end_it != line.end()) {
                    // Invalid sequence: stop the command list
                    const auto utf16_line_count = utf8::utf8to16(std::to_string(line_count));
                    return std::u16string(u"Invalid UTF-8 encoding detected at line ").append(utf16_line_count);
                }

                // Convert to utf16 then append to the cursor
                const auto u16string = utf8::utf8to16(line);
                command_list.emplace_back(u16string);
                ++line_count;
            }
            ifs.close();

            for (const auto& command : command_list) {
                // fixme?: At this point, any feedback needed will interrupt the command list execution
                // fixme?: autoexec does not show in history
                // todo: take in account "#" as comment and don't execute the line
                if (const auto result = execute(cursor, command); result.has_value()) {
                    return result.value();
                }
            }

            return std::nullopt;
    },
    [&](const int32_t argumentIndex, const std::string_view input, const ItemCallback<char>& itemCallback) {
       if (argumentIndex != 0) {
           // Only auto-complete the first argument (path)
           return;
       }
       getPathCompletions(input, false, itemCallback);
   });
}
