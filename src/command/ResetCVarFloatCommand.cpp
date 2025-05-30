#include "ResetCVarFloatCommand.h"


ResetCVarFloatCommand::ResetCVarFloatCommand(std::shared_ptr<CVarFloat> cvar)
    : m_cvar(std::move(cvar)) {}

void ResetCVarFloatCommand::provideAutoComplete(const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) argumentIndex;
    (void) input;
    (void) itemCallback;
    // No-op
}

std::optional<std::u16string> ResetCVarFloatCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    (void) payload;
    if (!args.empty()) {
        return u"Expected 0 argument.";
    }

    m_cvar->m_value = 0.0f;
    return std::nullopt;
}
