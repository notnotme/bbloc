#ifndef COMMAND_RUNNER_H
#define COMMAND_RUNNER_H

#include <string_view>

class CommandRunner {
public:
    virtual ~CommandRunner() = default;

    /**
     * @brief Run the said command input.
     * @param input The command to run, including the arguments.
     * @param fromPrompt true if that command is run from the command prompt.
     * @return true if the command was run, false otherwise.
     */
    virtual bool runCommand(std::u16string_view input, bool fromPrompt) = 0;

    /**
     * @brief Gathers auto-completion suggestions for command names.
     * @param input Current user input string.
     * @param itemCallback Callback to receive command name suggestions.
     */
    virtual void getCommandCompletions(std::string_view input, const AutoCompleteCallback<char> &itemCallback) = 0;

    /**
     * @brief Provides auto-completions for command arguments.
     * @param command The command name.
     * @param argumentIndex The index of the argument to complete.
     * @param input Current user input string.
     * @param itemCallback Callback to receive argument name suggestions.
     */
    virtual void getArgumentsCompletions(std::string_view command, int32_t argumentIndex, std::string_view input, const AutoCompleteCallback<char> &itemCallback) = 0;

    /**
      * @brief Provides completions for interactive feedback input.
      * @param itemCallback Callback receiving feedback suggestions.
      */
    virtual void getFeedbackCompletions(const AutoCompleteCallback<char16_t> &itemCallback) const = 0;
};


#endif //COMMAND_RUNNER_H
