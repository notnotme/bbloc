#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "../cursor/Cursor.h"
#include "cvar/CVar.h"
#include "cvar/CVarCallback.h"
#include "CommandCallback.h"
#include "CompletionCallback.h"
#include "FeedbackCallback.h"
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
        CommandCallback func;               ///< Function to execute for this command.
        CompletionCallback completion_func; ///< Optional function for argument auto-completion.
    };

    /** @brief Structure for managing command feedback interactions (e.g., confirmation prompts). */
    struct CommandFeedback final {
        std::u16string prompt;                   ///< Prompt message displayed to the user.
        std::u16string command;                  ///< Command associated with the feedback.
        std::vector<std::u16string> completions; ///< List of valid input options (e.g., "y", "n").
        FeedbackCallback on_validate_callback;   ///< Callback to run after receiving user input.
    };

    /** Registered commands. */
    std::unordered_map<std::string, CommandEntry> m_commands;

    /** Registered configuration variables. */
    std::unordered_map<std::string, CVarEntry> m_cvars;

    /** Active feedback prompt state. */
    std::optional<CommandFeedback> m_command_feedback;

    /** @brief Registers the built-in cvar command. */
    void registerCvarCommand();

    /** @brief Registers the built-in exec command. */
    void registerExecCommand();


public:
    /** @brief Deleted copy constructor. */
    CommandManager(const CommandManager &) = delete;

    /** @brief Deleted copy assignment operator. */
    CommandManager &operator=(const CommandManager &) = delete;

    /** @brief Constructs the CommandManager. */
    explicit CommandManager() = default;

    /** @brief Initializes the command manager and registers built-in commands. */
    void create();

    /** @brief Releases all internal resources and clears all registered items. */
    void destroy();

    /**
     * @brief Registers a new command.
     * @param name Command name (must be unique).
     * @param callback Function executed when the command is called.
     * @param completionCallback Optional function for argument auto-completion.
     */
    void registerCommand(std::string_view name, const CommandCallback& callback, const CompletionCallback& completionCallback = nullptr);

    /**
     * @brief Registers a new configuration variable (CVar).
     * @param name Variable name (must be unique).
     * @param cvar Shared pointer to the CVar instance.
     * @param callback Optional callback invoked when the variable is modified.
     */
    void registerCvar(std::string_view name, std::shared_ptr<CVar> cvar, const CVarCallback& callback = nullptr);

    /**
     * @brief Executes a command string.
     * @param cursor Reference to the cursor buffer.
     * @param input UTF-16 input string containing the command and arguments.
     * @return An optional result string for displaying messages in the prompt.
     */
    std::optional<std::u16string> execute(Cursor& cursor,std::u16string_view input);

    /**
     * @brief Initiates a feedback command. Displays a prompt to the user and waits for confirmation or selection.
     * @param prompt The message shown to the user.
     * @param command The command to be executed after feedback.
     * @param completions A list of valid responses (e.g., {"yes", "no"}).
     * @param callback Callback invoked with the user's feedback.
     */
    void setCommandFeedback(std::u16string_view prompt, std::u16string_view command, const std::vector<std::u16string_view>& completions, const FeedbackCallback& callback);

    /**
     * @brief Gathers auto-completion suggestions for CVars.
     * @param input The current input string.
     * @param itemCallback Callback to receive each CVar name suggestion.
     */
    void getCVarCompletions(std::string_view input, const ItemCallback<char>& itemCallback);

    /**
     * @brief Gathers auto-completion suggestions for command names.
     * @param input Current user input string.
     * @param itemCallback Callback to receive command name suggestions.
     */
    void getCommandCompletions(std::string_view input, const ItemCallback<char>& itemCallback);

    /**
     * @brief Provides auto-completions for command arguments.
     * @param command The command name.
     * @param argumentIndex The index of the argument to complete.
     * @param input Current user input string.
     * @param itemCallback Callback to receive argument name suggestions.
     */
    void getArgumentsCompletion(std::string_view command, int32_t argumentIndex, std::string_view input, const ItemCallback<char>& itemCallback);

    /**
      * @brief Provides completions for interactive feedback input.
      * @param itemCallback Callback receiving feedback suggestions.
      */
    void getFeedbackCompletion(const ItemCallback<char16_t>& itemCallback) const;

    /** @brief Clears any pending feedback prompt. */
    void clearCommandFeedback();

    /** return Optional prompt string to display to the user. */
    [[nodiscard]] std::optional<std::u16string_view> getCommandFeedback() const;

    /**
     * @brief Tokenizes a UTF-16 input string for command parsing. Splits the input into a list of arguments.
     * Quoted arguments are preserved as single tokens.
     * @param input The UTF-16 input string.
     * @return Vector of UTF-16 views representing each argument token.
     */
    [[nodiscard]] static std::vector<std::u16string_view> tokenize(std::u16string_view input);

    /**
     * @brief Gathers auto-completion suggestions for file system paths.
     * @param input The current input string (file or folder path).
     * @param foldersOnly If true, only folder names will be returned.
     * @param itemCallback Callback to receive each path suggestion.
     */
    static void getPathCompletions(std::string_view input, bool foldersOnly, const ItemCallback<char>& itemCallback);
};


#endif //COMMAND_MANAGER_H
