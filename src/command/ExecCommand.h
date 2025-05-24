#ifndef EXEC_COMMAND_H
#define EXEC_COMMAND_H

#include "../core/CursorContext.h"
#include "../core/base/Command.h"


class ExecCommand final : public Command<CursorContext> {
public:
    explicit ExecCommand() = default;
    void provideAutoComplete(int32_t argumentIndex, std::string_view input, const AutoCompleteCallback<char> &itemCallback) const override;
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;
};



#endif //EXEC_COMMAND_H
