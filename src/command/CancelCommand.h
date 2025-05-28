#ifndef CANCEL_COMMAND_H
#define CANCEL_COMMAND_H

#include <string>
#include <vector>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"
#include "../prompt/PromptState.h"


/**
 * @brief Command for canceling the prompt activation.
 *
 * This class implements the Command interface for canceling ongoing operations
 * such as an active prompt, or any other cancellable state.
 *
 * It primarily interacts with the prompt state to handle cancellation.
 */
class CancelCommand final : public Command<CursorContext> {
private:
    /** Reference to the prompt state this command will interact with. */
    PromptState &m_prompt_state;

public:
    /**
     * @brief Constructs a CancelCommand with the given prompt state.
     *
     * @param promptState Reference to the prompt state to interact with.
     */
    explicit CancelCommand(PromptState &promptState);

    /**
     * @brief Provides auto-completion suggestions for command arguments.
     *
     * This command does not expect any completion.
     *
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideAutoComplete(int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) const override;

    /**
     * @brief Executes the cancel operation.
     *
     * Cancels the current active operation, typically dismissing an active prompt.
     * Expect no arguments (empty vector).
     *
     * @param payload The cursor context at the time of cancellation.
     * @param args Command arguments (typically unused for cancel operations).
     * @return An optional message indicating the result of the cancellation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;

    /**
     * @brief Determines if the cancel command can be executed.
     *
     * Checks if there is an active operation that can be canceled in the current context.
     *
     * @param payload The cursor context to check for cancellable operations.
     * @return true if there is something to cancel, false otherwise.
     */
    [[nodiscard]] bool isRunnable(const CursorContext &payload) override;
};


#endif //CANCEL_COMMAND_H
