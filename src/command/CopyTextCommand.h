#ifndef COPY_TEXT_COMMAND_H
#define COPY_TEXT_COMMAND_H

#include <string>
#include <vector>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"


/**
 * @brief Command for copying text from the text editor to the clipboard.
 *
 * This class implements the Command interface for copying selected text
 * to the system clipboard.
 */
class CopyTextCommand final : public Command<CursorContext> {
public:
    /** @brief Constructs a CopyTextCommand with default initialization. */
    explicit CopyTextCommand() = default;

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
     * @brief Executes the copy operation.
     *
     * Copies the currently selected text to the system clipboard.
     * Expect 0 arguments (empty vector).
     *
     * @param payload The cursor context containing the current selection and document state.
     * @param args Command arguments that may modify the copy behavior.
     * @return An optional message indicating the result of the copy operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;

    /**
     * @brief Determines if the copy command can be executed.
     *
     * Checks if there is the selected text that can be copied in the current context.
     * Checks if the editor is focused.
     *
     * @param payload The cursor context to check for copyable content.
     * @return true if copying is possible, false otherwise.
     */
    [[nodiscard]] bool isRunnable(const CursorContext &payload) override;
};



#endif //COPY_TEXT_COMMAND_H
