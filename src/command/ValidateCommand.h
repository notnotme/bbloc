#ifndef VALIDATE_COMMAND_H
#define VALIDATE_COMMAND_H

#include <string>
#include <vector>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"
#include "../prompt/PromptState.h"


/**
 * @brief Command for validating and processing user input from the prompt.
 *
 * This class implements the Command interface for validating user input
 * entered in the prompt.
 */
class ValidateCommand final : public Command<CursorContext> {
private:
    /** @brief Reference to the prompt state this command will interact with. */
    PromptState &m_prompt_state;

public:
    /**
     * @brief Constructs a ValidateCommand with the specified prompt state.
     *
     * @param promptState Reference to the prompt state that will be modified during command execution.
     */
    explicit ValidateCommand(PromptState &promptState);

    /**
     * @brief Provides auto-completion suggestions for command arguments.
     *
     * Thos command does not auto-complete.
     *
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideAutoComplete(int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) const override;

    /**
     * @brief Executes the validation operation.
     *
     * Validates the user input from the prompt and processes it accordingly.
     * This command does not take arguments (empty vector).
     *
     * @param payload The cursor context containing the current editor state.
     * @param args Command arguments that may modify the validation behavior.
     * @return An optional message indicating the result of the validation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;

    /**
     * @brief Determines if the validate command can be executed.
     *
     * This command only runs if the prompt is focused.
     *
     * @param payload The cursor context to check.
     * @return true if validation is allowed, false otherwise.
     */
    [[nodiscard]] bool isRunnable(const CursorContext &payload) override;
};


#endif //VALIDATE_COMMAND_H
