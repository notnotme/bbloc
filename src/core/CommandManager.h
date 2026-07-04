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
#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "base/CVar.h"
#include "base/CVarCallback.h"
#include "base/Command.h"
#include "base/GlobalRegistry.h"
#include "base/AutoCompleteCallback.h"
#include "base/U16StringMap.h"
#include "CursorContext.h"
#include "CVarCommand.h"


/**
 * @brief Manages console commands and configuration variables (CVars).
 *
 * The CommandManager is responsible for registering, both commands and configuration variables. It provides a run
 * method that takes a payload and takes a vector of arguments to pass to the said command (if found) and can test if
 * the command can run with a payload.
 *
 * This does not inherit from CommandRunner because the logic of running a command is not tied to this class. This is
 * only a GlobalRegistry giving access to Command and CVar and some utility methods.
 */
class CommandManager final : public GlobalRegistry<CursorContext> {
private:
    /** Registered commands. */
    U16StringMap<std::shared_ptr<Command<CursorContext>>> m_commands;

    /** Names of commands excluded from prompt auto-completion. */
    std::unordered_set<std::u16string> m_hidden_commands;

    /** The CVarCommand */
    std::shared_ptr<CVarCommand> m_cvar_command;

public:
    /** @brief Deleted copy constructor. */
    CommandManager(const CommandManager &) = delete;

    /** @brief Deleted copy assignment operator. */
    CommandManager &operator=(const CommandManager &) = delete;

    /** @brief Clean allocated resources. */
    ~CommandManager() override = default;

    /** @brief Constructs the CommandManager. */
    explicit CommandManager();

    /**
     * @brief Registers a new command, optionally hidden from prompt auto-completion.
     *
     * Hidden commands only make sense when bound to a keystroke: they stay executable and are still
     * suggested when completions include hidden names (e.g. the command argument of bind).
     *
     * @param name Command name (must be unique).
     * @param command The command to register.
     * @param hidden If true, the command is excluded from prompt auto-completion.
     */
    void registerCommand(std::u16string_view name, std::shared_ptr<Command<CursorContext>> command, bool hidden) override;

    /**
     * @brief Registers a new configuration variable (CVar).
     *
     * @param name Variable name (must be unique).
     * @param cvar Shared pointer to the CVar instance.
     * @param callback Optional callback invoked when the variable is modified.
     */
    void registerCvar(std::u16string_view name, std::shared_ptr<CVar> cvar, const CVarCallback &callback) override;

    /**
     * @brief Executes a command string.
     *
     * @param payload Reference to the payload who run the command.
     * @param tokens List of UTF-16 input string view containing the command and arguments.
     * @return An optional result string for displaying messages in the prompt.
     */
    std::optional<std::u16string> run(CursorContext &payload, std::span<const std::u16string_view> tokens);

    /**
     * @brief Gathers auto-completion suggestions for command names.
     *
     * @param input Current user input string.
     * @param includeHidden If true, commands registered as hidden are suggested as well.
     * @param itemCallback Callback to receive command name suggestions.
     */
    void getCommandCompletions(std::u16string_view input, bool includeHidden, const AutoCompleteCallback &itemCallback);

    /**
     * @brief Provides auto-completions for command arguments.
     *
     * @param command The command name.
     * @param previousArgs The arguments typed before the one being completed, excluding the command name.
     * @param argumentIndex The index of the argument to complete.
     * @param input Current user input string.
     * @param itemCallback Callback to receive argument name suggestions.
     */
    void getArgumentsCompletion(std::u16string_view command, std::span<const std::u16string_view> previousArgs, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback);

    /**
     * @brief Tokenizes a UTF-16 input string for command parsing. Splits the input into a list of arguments.
     *
     * Quoted arguments are preserved as single tokens.
     *
     * @param input The UTF-16 input string.
     * @param tokens Receives the UTF-16 views of each argument token; cleared first, reusing its capacity.
     */
    static void tokenize(std::u16string_view input, std::vector<std::u16string_view> &tokens);

    /**
     * @brief Split a UTF-16 input string.
     *
     * @param input The UTF-16 input string.
     * @param delimiter The delimiter to use to split the string apart.
     * @return Vector of UTF-16 views representing each part.
     */
    [[nodiscard]] static std::vector<std::u16string_view> split(std::u16string_view input, char16_t delimiter);

    /**
     * @brief Gathers auto-completion suggestions for file system paths.
     *
     * @param input The current input string (file or folder path).
     * @param foldersOnly If true, only folder names will be returned.
     * @param itemCallback Callback to receive each path suggestion.
     */
    static void getPathCompletions(std::u16string_view input, bool foldersOnly, const AutoCompleteCallback &itemCallback);
};


#endif //COMMAND_MANAGER_H
