#ifndef SET_HIGH_LIGHT_COMMAND_H
#define SET_HIGH_LIGHT_COMMAND_H

#include "../core/CursorContext.h"
#include "../core/base/Command.h"


class SetHighLightCommand final : public Command<CursorContext> {
public:
    void provideAutoComplete(int32_t argumentIndex, std::string_view input, const AutoCompleteCallback<char> &itemCallback) const override;
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;
};


#endif //SET_HIGH_LIGHT_COMMAND_H
