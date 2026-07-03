#ifndef UNDO_COMMAND_H
#define UNDO_COMMAND_H

#include <string>
#include <vector>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"


/**
 * @brief Command for undoing the last text modification in the editor.
 *
 * This class implements the Command interface for undo operations,
 * restoring the buffer and cursor to their state before the last recorded edit.
 */
class UndoCommand final : public Command<CursorContext> {
public:
    /** @brief Constructs an UndoCommand with default initialization. */
    explicit UndoCommand() = default;

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
     * @brief Executes the undo operation.
     *
     * Restores the previous buffer snapshot and forwards the resulting edit to the highlighter.
     * This command expects 0 argument (empty Vector).
     *
     * @param payload The cursor context that will be modified by the undo operation.
     * @param args Command arguments (typically unused for undo operations).
     * @return An optional message indicating the result of the operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;
};


#endif //UNDO_COMMAND_H
