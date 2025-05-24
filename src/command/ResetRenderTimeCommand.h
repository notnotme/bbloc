#ifndef RESET_RENDER_TIME_COMMAND_H
#define RESET_RENDER_TIME_COMMAND_H

#include <utility>

#include "../core/CursorContext.h"
#include "../core/base/Command.h"
#include "../core/cvar/CVarFloat.h"


class ResetRenderTimeCommand final : public Command<CursorContext> {
private:
    std::shared_ptr<CVarFloat> m_render_time;

public:
    explicit ResetRenderTimeCommand(std::shared_ptr<CVarFloat> renderTimeCvar);
    void provideAutoComplete(int32_t argumentIndex, std::string_view input, const AutoCompleteCallback<char> &itemCallback) const override;
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, const std::vector<std::u16string_view> &args) override;
};




#endif //RESET_RENDER_TIME_COMMAND_H
