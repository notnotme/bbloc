#ifndef EXEC_COMMAND_H
#define EXEC_COMMAND_H

#include <string>
#include <vector>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"


/**
 * @brief Command for executing editor commands from a file.
 *
 * This class implements the Command interface for executing editor commands listed in a text file,
 * potentially capturing their output and integrating it with the editor.
 */
class ExecCommand final : public Command<CursorContext> {
    /** @brief Maximum number of nested exec calls before execution is refused. */
    static constexpr int32_t MAX_RECURSION_DEPTH = 8;

    int32_t m_recursion_depth = 0; ///< Current number of nested exec calls, guards against script cycles.

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
     * @param previousArgs The arguments typed before the one being completed, excluding the command name.
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideAutoComplete(const std::vector<std::u16string_view> &previousArgs, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) const override;

    /**
     * @brief Executes the editor commands read from a file.
     *
     * Read a text file and run the command line by line. Redirecting the output to the prompt if necessary.
     * Expect 1 argument which is the file path where to read the file with the list of commands.
     *
     * @param payload The cursor context at the point of execution.
     * @param args A single argument: the path to the file containing the commands to execute.
     * @return An optional message indicating the result of the exec operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;
};


#endif //EXEC_COMMAND_H
