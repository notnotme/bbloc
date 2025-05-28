#ifndef PASTE_TEXT_COMMAND_H
#define PASTE_TEXT_COMMAND_H

#include <string>
#include <vector>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"


/**
 * @brief Command for pasting text from the clipboard into the editor.
 *
 * This class implements the Command interface for pasting text operations,
 * allowing users to insert clipboard content at the current cursor position.
 */
class PasteTextCommand final : public Command<CursorContext> {
public:
    /** @brief Constructs a PasteTextCommand with default initialization. */
    explicit PasteTextCommand() = default;

    /**
      * @brief Provides auto-completion suggestions for command arguments.
      *
      * THos command does not auto-complete.
      *
      * @param argumentIndex The index of the argument currently being completed.
      * @param input The current partial input from the user for this argument.
      * @param itemCallback A callback to be invoked with each completion suggestion.
      */
    void provideAutoComplete(int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) const override;

    /**
     * @brief Executes the paste operation.
     *
     * Retrieves text from the system clipboard and inserts it at the
     * current cursor position in the document.
     * This command expects 0 argument (empty Vector).
     *
     * @param payload The cursor context that will be modified by the paste operation.
     * @param args Command arguments (typically unused for paste operations).
     * @return An optional message indicating the result of the operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;

    /**
     * @brief Determines if the paste command can be executed.
     *
     * Checks if the current context allows for pasting operations.
     * This command only runs if the focus is on the Editor.
     *
     * @param payload The cursor context to check for paste availability.
     * @return true if paste operation can be performed, false otherwise.
     */
    [[nodiscard]] bool isRunnable(const CursorContext &payload) override;
};


#endif //PASTE_TEXT_COMMAND_H
