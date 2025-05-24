#ifndef OPEN_FILE_COMMAND_H
#define OPEN_FILE_COMMAND_H

#include "../core/CursorContext.h"
#include "../core/base/Command.h"


class OpenFileCommand final : public Command<CursorContext> {
public:
    explicit OpenFileCommand() = default;
    void provideAutoComplete(int32_t argumentIndex, std::string_view input, const AutoCompleteCallback<char> &itemCallback) const override;
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;
};



#endif //OPEN_FILE_COMMAND_H
