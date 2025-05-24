#ifndef PASTE_TEXT_COMMAND_H
#define PASTE_TEXT_COMMAND_H

#include "../core/CursorContext.h"
#include "../core/base/Command.h"


class PasteTextCommand final : public Command<CursorContext> {
public:
    explicit PasteTextCommand() = default;
    void provideAutoComplete(int32_t argumentIndex, std::string_view input, const AutoCompleteCallback<char> &itemCallback) const override;
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;
    [[nodiscard]] bool isRunnable(const CursorContext &payload) override;
};



#endif //PASTE_TEXT_COMMAND_H
