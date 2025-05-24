#ifndef VALIDATE_COMMAND_H
#define VALIDATE_COMMAND_H

#include "../core/CursorContext.h"
#include "../core/base/Command.h"
#include "../prompt/PromptState.h"


class ValidateCommand final : public Command<CursorContext> {
private:
    PromptState &m_prompt_state;

public:
    explicit ValidateCommand(PromptState &promptState);
    void provideAutoComplete(int32_t argumentIndex, std::string_view input, const AutoCompleteCallback<char> &itemCallback) const override;
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;
    [[nodiscard]] bool isRunnable(const CursorContext &payload) override;
};



#endif //VALIDATE_COMMAND_H
