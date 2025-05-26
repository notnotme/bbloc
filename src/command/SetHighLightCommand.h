#ifndef SET_HIGH_LIGHT_COMMAND_H
#define SET_HIGH_LIGHT_COMMAND_H


#include <string>
#include <vector>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"


/**
 * @brief Command for setting highlighting options in the editor.
 *
 * This class implements the Command interface for controlling text highlighting
 * behavior in the editor. It allows users to toggle syntax highlighting.
 */
class SetHighLightCommand final : public Command<CursorContext> {
public:
    /** @brief Constructs a SetHighLightCommand with default initialization. */
    explicit SetHighLightCommand() = default;

    /**
     * @brief Provides auto-completion suggestions for highlighting options.
     *
     * This command auto-completes argument 0 which is the highlight mode.
     *
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideAutoComplete(int32_t argumentIndex, std::string_view input, const AutoCompleteCallback<char> &itemCallback) const override;

    /**
     * @brief Executes the highlighting command.
     *
     * Changes the highlighting behavior based on the provided arguments.
     * Expect 1 argument which is the parser name.
     * See Highlighter::PARSERS for potential arguments.
     *
     * @param payload The cursor context containing the editor state to be modified.
     * @param args Command arguments that specify the highlighting options to apply.
     * @return An optional message indicating the result of the operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;
};


#endif //SET_HIGH_LIGHT_COMMAND_H
