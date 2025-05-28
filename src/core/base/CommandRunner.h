#ifndef COMMAND_RUNNER_H
#define COMMAND_RUNNER_H

#include <string_view>


/**
 * @brief Abstract interface for executing user commands and providing auto-completion suggestions.
 *
 * This class handles parsing, dispatching, and completing console commands entered
 * by users in the prompt. Implementations should manage command execution flow and
 * intelligent suggestions for command names, arguments, and feedback.
 */
class CommandRunner {
public:
    virtual ~CommandRunner() = default;

    /**
     * @brief Executes the provided command input.
     *
     * @param input The full command input string, including arguments.
     * @param fromPrompt Indicates whether the input came from the interactive command prompt.
     * @return true if a command was successfully executed; false if unrecognized.
     */
    virtual bool runCommand(std::u16string_view input, bool fromPrompt) = 0;

    /**
     * @brief Provides auto-completion suggestions for command names.
     *
     * @param input The current (partial) user input string.
     * @param itemCallback Callback to receive possible command name completions.
     */
    virtual void getCommandCompletions(std::u16string_view input, const AutoCompleteCallback &itemCallback) = 0;

    /**
     * @brief Provides auto-completion suggestions for command arguments.
     *
     * @param command The name of the command being executed.
     * @param argumentIndex The zero-based index of the argument currently being completed.
     * @param input The current (partial) user input for this argument.
     * @param itemCallback Callback to receive possible argument completions.
     */
    virtual void getArgumentsCompletions(std::u16string_view command, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) = 0;
};


#endif //COMMAND_RUNNER_H
