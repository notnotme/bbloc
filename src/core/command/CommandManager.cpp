#include "CommandManager.h"

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <utf8.h>
#include <ranges>
#include <vector>


CommandManager::CommandManager() {
    registerCVarCommand();
}

void CommandManager::registerCommand(const std::string_view name, const ConditionCallback &conditionCallback, const CommandCallback &commandCallback, const CompletionCallback &completionCallback) {
    const auto c_string = name.data();
    if (m_commands.contains(c_string)) {
        throw std::runtime_error(std::string("Command already registered: ").append(name));
    }

    const auto &[new_entry, success] = m_commands.insert({ c_string, { conditionCallback, commandCallback, completionCallback } });
    if (!success) {
        throw std::runtime_error(std::string("Unable to register command: ").append(name));
    }
}

void CommandManager::registerCvar(const std::string_view name, std::shared_ptr<CVar> cvar, const CVarCallback &callback) {
    const auto c_string = name.data();
    if (m_cvars.contains(c_string)) {
        throw std::runtime_error(std::string("CVar already registered: ").append(name));
    }

    const auto &[new_entry, success] = m_cvars.insert({c_string, { .cvar = std::move(cvar), .callback = callback}});
    if (!success) {
        throw std::runtime_error(std::string("Unable to register  CVar: ").append(name));
    }
}

std::optional<std::u16string> CommandManager::execute(CursorContext &context, const std::vector<std::u16string_view> &tokens) {
    if (tokens.empty()) {
        // Nothing to process
        return std::nullopt;
    }

    const auto command = utf8::utf16to8(tokens[0]);
    if (const auto &cmd = m_commands.find(command); cmd != m_commands.end()) {
        // Skip the first item in the tokens, as it is the command name and we don't need it
        return cmd->second.command_callback(context, { tokens.begin() + 1, tokens.end() });
    }

    return std::u16string(u"Unknown command: ").append(tokens[0]);
}

void CommandManager::getCommandCompletions(const std::string_view input, const ItemCallback<char> &itemCallback) {
    const auto input_is_empty = input.empty();
    for (const auto &name : std::views::keys(m_commands)) {
        if (name.starts_with(input) || input_is_empty) {
            // If input is empty, push everything to the list of possibilities
            itemCallback(name);
        }
    }
}

void CommandManager::getCVarCompletions(const std::string_view input, const ItemCallback<char> &itemCallback) {
    const auto input_is_empty = input.empty();
    for (const auto &name : std::views::keys(m_cvars)) {
        if (name.starts_with(input) || input_is_empty) {
            // If input is empty, push everything to the list of possibilities
            itemCallback(name);
        }
    }
}

void CommandManager::getArgumentsCompletion(const CursorContext &context, const std::string_view command, const int32_t argumentIndex, const std::string_view input, const ItemCallback<char> &itemCallback) {
    if (const auto &cmd = m_commands.find(command.data()); cmd != m_commands.end()) {
        if (cmd->second.completion_func != nullptr) {
            cmd->second.completion_func(context, argumentIndex, input, itemCallback);
        }
    }
}

bool CommandManager::canExecute(const std::vector<std::u16string_view> &tokens) {
    if (tokens.empty()) {
        // Nothing to process
        return false;
    }

    const auto utf8_command = utf8::utf16to8(tokens[0]);
    if (const auto &cmd = m_commands.find(utf8_command); cmd != m_commands.end()) {
        return cmd->second.condition_callback(tokens);
    }

    return false;
}

void CommandManager::getPathCompletions(const std::string_view input, const bool foldersOnly, const ItemCallback<char> &itemCallback) {
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

void CommandManager::registerCVarCommand() {
    // Register a "cvar" command that prints or set a CVar value
    // This will also call any callback registered with the said CVar
    // built-in CommandManager
    registerCommand("cvar",
        [](const std::vector<std::u16string_view> &commandTokens) {
            (void) commandTokens;
            // This can be run with no restriction
            return true;
        },
        [&](const CursorContext &context, const std::vector<std::u16string_view> &args) -> std::optional<std::u16string> {
            (void) context;
            if (args.empty()) {
                return u"Usage: cvar <name> [value1] [value2] ...";
            }

            const auto cvar_name = utf8::utf16to8(args[0]);
            if (!m_cvars.contains(cvar_name)) {
                return std::u16string(u"Unknown cvar: ").append(args[0]);
            }

            const auto &cvar_entry = m_cvars[cvar_name];
            auto *cvar = cvar_entry.cvar.get();

            if (args.size() == 1) {
                // Print the value of this cvar
                return std::u16string(args[0]).append(u": ").append(cvar->getStringValue());
            }

            // Se the value of this cvar if not read-only
            if (cvar->isReadOnly()) {
                return std::u16string(u"CVar is read-only: ").append(args[0]);
            }

            if (const auto &error = cvar->setValueFromStrings({args.begin() + 1, args.end()})) {
                // Something wrong happened
                return std::u16string(args[0]).append(u": ").append(error.value());
            }

            // Eventually invoke the associated callback
            if (const auto &callback = cvar_entry.callback) {
                callback();
            }

            return std::nullopt;
        },
        [&](const CursorContext &context, const int32_t argumentIndex, const std::string_view input, const ItemCallback<char> &itemCallback) {
            (void) context;
            if (argumentIndex > 0) {
                // Only auto-complete the names of CVars
                return;
            }
            getCVarCompletions(input, itemCallback);
        });
}
