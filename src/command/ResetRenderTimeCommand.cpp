#include "ResetRenderTimeCommand.h"

ResetRenderTimeCommand::ResetRenderTimeCommand(std::shared_ptr<CVarFloat> renderTimeCvar)
    : m_render_time(std::move(renderTimeCvar)) {}

void ResetRenderTimeCommand::provideAutoComplete(const int32_t argumentIndex, const std::string_view input, const AutoCompleteCallback<char> &itemCallback) const {
    (void) argumentIndex;
    (void) input;
    (void) itemCallback;
    // No-op
}

std::optional<std::u16string> ResetRenderTimeCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    (void) payload;
    if (!args.empty()) {
        return u"Expected 0 argument.";
    }

    m_render_time->m_value = 0.0f;
    return std::nullopt;
}
