#ifndef FONT_SIZE_COMMAND_H
#define FONT_SIZE_COMMAND_H

#include <string>
#include <vector>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"


/**
 * @brief Command for adjusting the font size in the application.
 *
 * This class implements the Command interface for changing the display font size
 * of the application. It allows users to increase, decrease, or set the font size to
 * a specific value to improve readability based on their preferences.
 */
class FontSizeCommand final : public Command<CursorContext> {
public:
    /** @brief Constructs a FontSizeCommand with default initialization. */
    explicit FontSizeCommand() = default;

    /**
     * @brief Provides auto-completion suggestions for command arguments.
     *
     * This command auto-completes the first argument with "+" or "-".
     *
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideAutoComplete(int32_t argumentIndex, std::string_view input, const AutoCompleteCallback<char> &itemCallback) const override;

    /**
     * @brief Executes the font size adjustment.
     *
     * Changes the editor's font size based on the provided arguments. Which can be:
     * - "+" to increase the font size by one unit.
     * - "-" to decrease the font size by one unit.
     * - "x" which is an integer value to set the font size to a defined value directly.
     *
     * The font size may be clamped by the application if it does not fit certain condition.
     *
     * @param payload The cursor context.
     * @param args Command arguments specifying how to adjust the font size.
     * @return An optional message indicating the new font size or the result of the operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;
};



#endif //FONT_SIZE_COMMAND_H
