#ifndef ACTIVATE_PROMPT_COMMAND_H
#define ACTIVATE_PROMPT_COMMAND_H

#include <string>
#include <vector>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"
#include "../prompt/PromptState.h"


/**
 * @brief Command that activates the prompt functionality in the text editor.
 *
 * This class implements the Command interface for activating and managing
 * the prompt within the editor. It handles the interaction between user input
 * and the prompt state, providing auto-completion suggestions and validating
 * when the command can be executed.
 */
class ActivatePromptCommand final : public Command<CursorContext> {
private:
    /** Reference to the prompt state this command will manipulate. */
    PromptState &m_prompt_state;

public:
    /**
     * @brief Constructs an ActivatePromptCommand with the given prompt state.
     *
     * @param promptState Reference to the prompt state to be activated and managed.
     */
    explicit ActivatePromptCommand(PromptState &promptState);

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
     * @brief Executes the prompt activation command.
     *
     * Activates the prompt with the given context. The args vector must be empty.
     *
     * @param payload The cursor context to use when activating the prompt.
     * @param args Command arguments. An empty vector is expected.
     * @return An optional message indicating the result of the operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;

    /**
     * @brief Determines if the prompt activation command can be executed.
     *
     * Checks if the current cursor context allows for prompt activation.
     * Only the editor can activate the prompt (this command works via key-binding), it has no effect inside the prompt
     * itself.
     *
     * @param payload The cursor context to check.
     * @return true if the prompt can be activated, false otherwise.
     */
    [[nodiscard]] bool isRunnable(const CursorContext &payload) override;
};


#endif //ACTIVATE_PROMPT_COMMAND_H
