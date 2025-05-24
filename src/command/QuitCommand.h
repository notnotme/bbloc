#ifndef QUIT_COMMAND_H
#define QUIT_COMMAND_H

#include "../core/CursorContext.h"
#include "../core/base/Command.h"


class QuitCommand final : public Command<CursorContext> {
public:
    explicit QuitCommand() = default;
    void provideAutoComplete(int32_t argumentIndex, std::string_view input, const AutoCompleteCallback<char> &itemCallback) const override;
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;
};



#endif //QUIT_COMMAND_H
