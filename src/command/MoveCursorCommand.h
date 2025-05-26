#ifndef MOVE_CURSOR_COMMAND_H
#define MOVE_CURSOR_COMMAND_H

#include <string>
#include <vector>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"
#include "../prompt/PromptState.h"


/**
 * @brief Command for moving the cursor within the text editor.
 *
 * This class implements the Command interface for cursor navigation,
 * allowing users to move the cursor to predefined positions within the document.
 */
class MoveCursorCommand final : public Command<CursorContext> {
private:
    /** Reference to the prompt state. */
    PromptState &m_prompt_state;

public:
    /**
     * @brief Constructs a MoveCursorCommand with a reference to the prompt state.
     *
     * @param promptState Reference to the current prompt state for context-aware navigation.
     */
    explicit MoveCursorCommand(PromptState &promptState);

    /**
     * @brief Provides auto-completion suggestions for command arguments.
     *
     * This command auto-completes the first and second arguments.
     * The first is the direction which can one of: "up", "down", "left", "right", "page_up", "page_down", "bof", "eof",
     * "bol" or "eol".
     * The second is "true" or "false", which activates / extends the selection or cancels it.
     *
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideAutoComplete(int32_t argumentIndex, std::string_view input, const AutoCompleteCallback<char> &itemCallback) const override;

    /**
     * @brief Executes the cursor movement.
     *
     * Moves the cursor within the document based on the provided arguments.
     * The first is the direction that can one of: "up", "down", "left", "right", "page_up", "page_down", "bof", "eof",
     * "bol" or "eol".
     * The second is "true" or "false", which activates / extends the selection or cancels it.
     *
     * @param payload The cursor context that will be modified by this command.
     * @param args Command arguments specifying how to move the cursor.
     * @return An optional message indicating the new cursor position or the result of the operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;
};



#endif //MOVE_CURSOR_COMMAND_H
