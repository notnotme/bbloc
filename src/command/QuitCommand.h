#ifndef QUIT_COMMAND_H
#define QUIT_COMMAND_H

#include <string>
#include <vector>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"


/**
 * @brief Command for exiting the application.
 *
 * This class implements the Command interface for quitting/exiting the application.
 * It handles the graceful shutdown of the editor, performing cleanup operations before termination.
 *
 * This does not check if modified files are saved to disk.
 */
class QuitCommand final : public Command<CursorContext> {
public:
    /** @brief Constructs a QuitCommand with default initialization. */
    explicit QuitCommand() = default;

    /**
     * @brief Provides auto-completion suggestions for command arguments.
     *
     * This command does not auto-complete.
     *
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideAutoComplete(int32_t argumentIndex, std::string_view input, const AutoCompleteCallback<char> &itemCallback) const override;

    /**
     * @brief Executes the quit operation.
     *
     * Initiates the application shutdown process.
     * This command expects no arguments (empty Vector).
     *
     * @param payload The cursor context that may be checked for unsaved changes.
     * @param args Command arguments that may modify the quit behavior (e.g., "force" to skip confirmation).
     * @return An optional message indicating the result of the operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;
};



#endif //QUIT_COMMAND_H
