#ifndef RESET_CVAR_FLOAT_COMMAND_H
#define RESET_CVAR_FLOAT_COMMAND_H

#include <string>
#include <vector>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"
#include "../core/cvar/CVarFloat.h"


/**
 * @brief Command for resetting a floating-point configuration variable to its default value.
 *
 * This class implements the Command interface for resetting float-type CVars to 0.
 */
class ResetCVarFloatCommand final : public Command<CursorContext> {
private:
    /** Reference to the float CVar that this command will manage. */
    std::shared_ptr<CVarFloat> m_cvar;

public:
    /** @brief Constructs a ResetCVarFloatCommand with default initialization. */
    explicit ResetCVarFloatCommand(std::shared_ptr<CVarFloat> cvar);

    /**
     * @brief Provides auto-completion suggestions for CVar names.
     *
     * This command does not auto-complete.
     *
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideAutoComplete(int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) const override;

    /**
     * @brief Executes the CVar reset operation.
     *
     * Resets the specified float-type CVar to its default value.
     * This command expect no argument (empty vector).
     *
     * @param payload The cursor context (not directly used for CVar operations).
     * @param args Command arguments, with the first argument being the name of the CVar to reset.
     * @return An optional message indicating the result of the operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;
};


#endif //RESET_CVAR_FLOAT_COMMAND_H
