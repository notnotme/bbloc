#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "base/CVar.h"
#include "base/CVarCallback.h"
#include "base/Command.h"
#include "base/GlobalRegistry.h"
#include "base/AutoCompleteCallback.h"
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
    std::unordered_map<std::u16string, std::shared_ptr<Command<CursorContext>>> m_commands;

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
     * @brief Registers a new command.
     *
     * @param name Command name (must be unique).
     * @param command The command to register.
     */
    void registerCommand(std::u16string_view name, std::shared_ptr<Command<CursorContext>> command) override;

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
    std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &tokens);

    /**
     * @brief Gathers auto-completion suggestions for command names.
     *
     * @param input Current user input string.
     * @param itemCallback Callback to receive command name suggestions.
     */
    void getCommandCompletions(std::u16string_view input, const AutoCompleteCallback &itemCallback);

    /**
     * @brief Provides auto-completions for command arguments.
     *
     * @param command The command name.
     * @param argumentIndex The index of the argument to complete.
     * @param input Current user input string.
     * @param itemCallback Callback to receive argument name suggestions.
     */
    void getArgumentsCompletion(std::u16string_view command, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback);

    /**
     * @brief Check if a command can be run.
     *
     * @param payload A payload to help decide.
     * @param name The name of the command to test.
     * @return An optional result string for displaying messages in the prompt.
     */
    [[nodiscard]] bool isRunnable(const CursorContext &payload, std::u16string_view name);

    /**
     * @brief Tokenizes a UTF-16 input string for command parsing. Splits the input into a list of arguments.
     *
     * Quoted arguments are preserved as single tokens.
     * fixme: This does not play well with auto-complete
     *
     * @param input The UTF-16 input string.
     * @return Vector of UTF-16 views representing each argument token.
     */
    [[nodiscard]] static std::vector<std::u16string_view> tokenize(std::u16string_view input);

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
