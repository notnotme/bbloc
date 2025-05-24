#ifndef COPY_TEXT_COMMAND_H
#define COPY_TEXT_COMMAND_H

#include "../core/CursorContext.h"
#include "../core/base/Command.h"


class CopyTextCommand final : public Command<CursorContext> {
public:
    explicit CopyTextCommand() = default;
    void provideAutoComplete(int32_t argumentIndex, std::string_view input, const AutoCompleteCallback<char> &itemCallback) const override;
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;
    [[nodiscard]] bool isRunnable(const CursorContext &payload) override;
};



#endif //COPY_TEXT_COMMAND_H
