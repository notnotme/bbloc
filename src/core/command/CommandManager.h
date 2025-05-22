#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "../CursorContext.h"
#include "cvar/CVar.h"
#include "cvar/CVarCallback.h"
#include "CommandCallback.h"
#include "CompletionCallback.h"
#include "ConditionCallback.h"
#include "ItemCallback.h"


/**
 * @brief Manages console commands and configuration variables (CVars).
 *
 * The CommandManager is responsible for registering, executing, and auto-completing
 * both commands and configuration variables. It also handles interactive feedback
 * prompts and supports file path completion utilities.
 */
class CommandManager final {
private:
    /** @brief Internal structure representing a configuration variable entry. */
    struct CVarEntry final {
        std::shared_ptr<CVar> cvar; ///< Pointer to the CVar instance.
        CVarCallback callback;      ///< Callback executed when the CVar value is changed.
    };

    /** @brief Internal structure representing a registered command. */
    struct CommandEntry final {
        ConditionCallback condition_callback;   ///< Optional callback to evaluate before command execution.
        CommandCallback command_callback;       ///< Function to execute for this command.
        CompletionCallback completion_func;     ///< Optional function for argument auto-completion.
    };

    /** Registered commands. */
    std::unordered_map<std::string, CommandEntry> m_commands;

    /** Registered configuration variables. */
    std::unordered_map<std::string, CVarEntry> m_cvars;

    /** @brief Registers the built-in cvar command. */
    void registerCVarCommand();

public:
    /** @brief Deleted copy constructor. */
    CommandManager(const CommandManager &) = delete;

    /** @brief Deleted copy assignment operator. */
    CommandManager &operator=(const CommandManager &) = delete;

    /** @brief Clean allocated resources. */
    ~CommandManager() = default;

    /** @brief Constructs the CommandManager. */
    explicit CommandManager();

    /**
     * @brief Registers a new command.
     * @param name Command name (must be unique).
     * @param conditionCallback Callback to evaluate if a command can be run.
     * @param commandCallback Function executed when the command is called.
     * @param completionCallback Optional function for argument auto-completion.
     */
    void registerCommand(std::string_view name, const ConditionCallback &conditionCallback, const CommandCallback &commandCallback, const CompletionCallback &completionCallback = nullptr);

    /**
     * @brief Registers a new configuration variable (CVar).
     * @param name Variable name (must be unique).
     * @param cvar Shared pointer to the CVar instance.
     * @param callback Optional callback invoked when the variable is modified.
     */
    void registerCvar(std::string_view name, std::shared_ptr<CVar> cvar, const CVarCallback &callback = nullptr);

    /**
     * @brief Executes a command string.
     * @param context Reference to the cursor context executing this command.
     * @param tokens List of UTF-16 input string view containing the command and arguments.
     * @return An optional result string for displaying messages in the prompt.
     */
    std::optional<std::u16string> execute(CursorContext &context, const std::vector<std::u16string_view> &tokens);

    /**
     * @brief Gathers auto-completion suggestions for CVars.
     * @param input The current input string.
     * @param itemCallback Callback to receive each CVar name suggestion.
     */
    void getCVarCompletions(std::string_view input, const ItemCallback<char> &itemCallback);

    /**
     * @brief Gathers auto-completion suggestions for command names.
     * @param input Current user input string.
     * @param itemCallback Callback to receive command name suggestions.
     */
    void getCommandCompletions(std::string_view input, const ItemCallback<char> &itemCallback);

    /**
     * @brief Provides auto-completions for command arguments.
     * @param context Reference to the cursor context.
     * @param command The command name.
     * @param argumentIndex The index of the argument to complete.
     * @param input Current user input string.
     * @param itemCallback Callback to receive argument name suggestions.
     */
    void getArgumentsCompletion(const CursorContext &context, std::string_view command, int32_t argumentIndex, std::string_view input, const ItemCallback<char> &itemCallback);

    /**
     * @brief Executes the condition function of a command string.
     * @param tokens List of UTF-16 input string view containing the command and arguments.
     * @return An optional result string for displaying messages in the prompt.
     */
    [[nodiscard]] bool canExecute(const std::vector<std::u16string_view> &tokens);

    /**
     * @brief Tokenizes a UTF-16 input string for command parsing. Splits the input into a list of arguments.
     * Quoted arguments are preserved as single tokens.
     * @param input The UTF-16 input string.
     * @return Vector of UTF-16 views representing each argument token.
     */
    [[nodiscard]] static std::vector<std::u16string_view> tokenize(std::u16string_view input);

    /**
     * @brief Split a UTF-16 input string.
     * @param input The UTF-16 input string.
     * @param delimiter The delimiter to use to split the string apart.
     * @return Vector of UTF-16 views representing each part.
     */
    [[nodiscard]] static std::vector<std::u16string_view> split(std::u16string_view input, char16_t delimiter);

    /**
     * @brief Gathers auto-completion suggestions for file system paths.
     * @param input The current input string (file or folder path).
     * @param foldersOnly If true, only folder names will be returned.
     * @param itemCallback Callback to receive each path suggestion.
     */
    static void getPathCompletions(std::string_view input, bool foldersOnly, const ItemCallback<char> &itemCallback);
};


#endif //COMMAND_MANAGER_H
