#ifndef AUTO_COMPLETE_COMMAND_H
#define AUTO_COMPLETE_COMMAND_H

#include <string>
#include <vector>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"
#include "../prompt/PromptState.h"


/**
 * @brief Command that triggers and manages auto-completion functionality in the text editor.
 *
 * This class implements the Command interface for handling auto-completion operations.
 * It interacts with the prompt state to provide contextual completion suggestions
 * based on the current cursor text and context.
 */
class AutoCompleteCommand final : public Command<CursorContext> {
private:
    /** Reference to the prompt state this command will use for auto-completion. */
    PromptState &m_prompt_state;

public:
    /**
     * @brief Constructs an AutoCompleteCommand with the given prompt state.
     *
     * @param promptState Reference to the prompt state to use for auto-completion.
     */
    explicit AutoCompleteCommand(PromptState &promptState);

    /**
     * @brief Provides auto-completion suggestions for command arguments.
     *
     * This command does not expect any completion.
     *
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideAutoComplete(int32_t argumentIndex, std::string_view input, const AutoCompleteCallback<char> &itemCallback) const override;

    /**
     * @brief Executes the auto-completion command.
     *
     * Triggers the auto-completion mechanism based on the current cursor context.
     * 0 or 1 argument are allowed: "forward" and "backward". "forward" being the default, if not specified.
     *
     * @param payload The cursor context containing position and document information.
     * @param args Command arguments that may modify the auto-completion behavior.
     * @return An optional message indicating the result of the operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;

    /**
     * @brief Determines if the auto-completion command can be executed.
     *
     * Checks if auto-completion is available in the current context.
     *
     * @param payload The cursor context to check for auto-completion availability.
     * @return true if auto-completion can be triggered, false otherwise.
     */
    [[nodiscard]] bool isRunnable(const CursorContext &payload) override;
};



#endif //AUTO_COMPLETE_COMMAND_H
