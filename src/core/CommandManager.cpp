#include "CommandManager.h"


CommandManager::CommandManager()
    : m_cvar_command(std::make_shared<CVarCommand>()) {
    m_commands.insert({ "cvar", m_cvar_command });
}

void CommandManager::registerCommand(const std::string_view name, std::shared_ptr<Command<CursorContext>> command) {
    const auto c_string = name.data();
    if (m_commands.contains(c_string)) {
        throw std::runtime_error(std::string("Command already registered: ").append(name));
    }

    const auto &[new_entry, success] = m_commands.insert({ c_string, command });
    if (!success) {
        throw std::runtime_error(std::string("Unable to register command: ").append(name));
    }
}

void CommandManager::registerCvar(const std::string_view name, std::shared_ptr<CVar> cvar, const CVarCallback &callback) {
    m_cvar_command->registerCvar(name, cvar, callback);
}

std::optional<std::u16string> CommandManager::run(CursorContext &payload, const std::vector<std::u16string_view> &tokens) {
    if (tokens.empty()) {
        // Nothing to process
        return std::nullopt;
    }

    const auto command = utf8::utf16to8(tokens[0]);
    if (const auto &cmd = m_commands.find(command); cmd != m_commands.end()) {
        // Skip the first item in the tokens, as it is the command name and we don't need it
        return cmd->second->run(payload, { tokens.begin() + 1, tokens.end() });
    }

    return std::u16string(u"Unknown command: ").append(tokens[0]);
}

void CommandManager::getCommandCompletions(const std::string_view input, const AutoCompleteCallback<char> &itemCallback) {
    const auto input_is_empty = input.empty();
    for (const auto &name : std::views::keys(m_commands)) {
        if (name.starts_with(input) || input_is_empty) {
            // If input is empty, push everything to the list of possibilities
            itemCallback(name);
        }
    }
}

void CommandManager::getArgumentsCompletion(const std::string_view command, const int32_t argumentIndex, const std::string_view input, const AutoCompleteCallback<char> &itemCallback) {
    if (const auto &cmd = m_commands.find(command.data()); cmd != m_commands.end()) {
        cmd->second->provideAutoComplete(argumentIndex, input, itemCallback);
    }
}

bool CommandManager::isRunnable(const CursorContext &payload, const std::string_view name) {
    if (const auto &cmd = m_commands.find(name.data()); cmd != m_commands.end()) {
        return cmd->second->isRunnable(payload);
    }

    // Let the command pass through if we don't know it, so if this command name tries to "run", it will
    // end up with the unknown command message in return.
    return true;
}

void CommandManager::getPathCompletions(const std::string_view input, const bool foldersOnly, const AutoCompleteCallback<char> &itemCallback) {
    const auto path_input = std::filesystem::path(input);
    const auto path_input_string = path_input.filename().string();
    const auto parent_path = path_input.has_parent_path() ? path_input.parent_path() : ".";
    const auto exists = std::filesystem::exists(parent_path);
    const auto is_directory = std::filesystem::is_directory(parent_path);
    if (exists && is_directory) {
        for (const auto &entry : std::filesystem::directory_iterator(parent_path)) {
            const auto &path = entry.path();
            const auto &complete_path = path.string();
            const auto &filename = path.filename().string();
            if (filename.starts_with(path_input_string)) {
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

std::vector<std::u16string_view> CommandManager::split(const std::u16string_view input, const char16_t delimiter) {
    std::vector<std::u16string_view> parts;
    auto start = 0;
    auto index = 0;
    while (index < input.length()) {
        if (input[index] == delimiter) {
            ++index;
            continue;
        }

        start = index;
        while (index < input.size() && input[index] != delimiter) {
            ++index;
        }
        parts.emplace_back(input.substr(start, index - start));
    }

    return parts;
}
