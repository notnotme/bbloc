#ifndef EXEC_COMMAND_H
#define EXEC_COMMAND_H

#include <string>
#include <vector>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"


/**
 * @brief Command for executing external commands in the text editor.
 *
 * This class implements the Command interface for executing commands inside a text file,
 * potentially capturing their output and integrating it with the editor.
 */
class ExecCommand final : public Command<CursorContext> {
public:
    /** @brief Constructs an ExecCommand with default initialization. */
    explicit ExecCommand() = default;

    /**
     * @brief Provides auto-completion suggestions for command arguments.
     *
     * Implements the Command interface method to suggest completions for
     * the exec command's arguments.
     *
     * Expect auto complete for the first argument, which is the file where to find the list of commands.
     *
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideAutoComplete(int32_t argumentIndex, std::string_view input, const AutoCompleteCallback<char> &itemCallback) const override;

    /**
     * @brief Executes the external command.
     *
     * Read a text file and run the command line by line. Redirecting the output to the prompt if necessary.
     * Expect 1 argument which is the file path where to read the file with the list of commands.
     *
     * @param payload The cursor context at the point of execution.
     * @param args Command arguments specifying the external command to run and its parameters.
     * @return An optional message indicating the result of the exec operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;
};



#endif //EXEC_COMMAND_H
