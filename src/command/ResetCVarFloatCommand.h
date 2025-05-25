#ifndef RESET_CVAR_FLOAT_COMMAND_H
#define RESET_CVAR_FLOAT_COMMAND_H

#include "../core/CursorContext.h"
#include "../core/base/Command.h"
#include "../core/cvar/CVarFloat.h"


class ResetCVarFloatCommand final : public Command<CursorContext> {
private:
    std::shared_ptr<CVarFloat> m_cvar;

public:
    explicit ResetCVarFloatCommand(std::shared_ptr<CVarFloat> cvar);
    void provideAutoComplete(int32_t argumentIndex, std::string_view input, const AutoCompleteCallback<char> &itemCallback) const override;
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;
};




#endif //RESET_CVAR_FLOAT_COMMAND_H
