#include "CancelCommand.h"

CancelCommand::CancelCommand(PromptState &promptState)
    : m_prompt_state(promptState) {}

void CancelCommand::provideAutoComplete(const int32_t argumentIndex, const std::string_view input, const AutoCompleteCallback<char> &itemCallback) const {
    (void) input;
    (void) argumentIndex;
    (void) itemCallback;
    // No-op
}

std::optional<std::u16string> CancelCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    (void) payload;
    if (!args.empty()) {
        return u"Expected 0 argument.";
    }

    switch (payload.focus_target) {
        case FocusTarget::Prompt:
            // Reset completions, history index, and feedback if the user quit the prompt
            m_prompt_state.setRunningState(PromptState::RunningState::Idle);
            break;
        case FocusTarget::Editor:
            // No-op
        default:
            break;
    }

    return std::nullopt;
}

bool CancelCommand::isRunnable(const CursorContext &payload) {
    return payload.focus_target == FocusTarget::Prompt;
}
