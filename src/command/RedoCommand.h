#ifndef REDO_COMMAND_H
#define REDO_COMMAND_H

#include <string>
#include <vector>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"


/**
 * @brief Command for re-applying the last undone text modification in the editor.
 *
 * This class implements the Command interface for redo operations,
 * restoring the buffer and cursor to their state before the last undo.
 */
class RedoCommand final : public Command<CursorContext> {
public:
    /** @brief Constructs a RedoCommand with default initialization. */
    explicit RedoCommand() = default;

    /**
      * @brief Provides auto-completion suggestions for command arguments.
      *
      * This command does not auto-complete.
      *
      * @param argumentIndex The index of the argument currently being completed.
      * @param input The current partial input from the user for this argument.
      * @param itemCallback A callback to be invoked with each completion suggestion.
      */
    void provideAutoComplete(int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) const override;

    /**
     * @brief Executes the redo operation.
     *
     * Restores the last undone buffer snapshot and forwards the resulting edit to the highlighter.
     * This command expects 0 argument (empty Vector).
     *
     * @param payload The cursor context that will be modified by the redo operation.
     * @param args Command arguments (typically unused for redo operations).
     * @return An optional message indicating the result of the operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;
};


#endif //REDO_COMMAND_H
