#include "CancelCommand.h"


CancelCommand::CancelCommand(PromptState &promptState)
    : m_prompt_state(promptState) {}

void CancelCommand::provideAutoComplete(const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
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

    // Set the prompt state to Idle, then the command processing logic will take care of the rest.
    m_prompt_state.setRunningState(PromptState::RunningState::Idle);

    return std::nullopt;
}

bool CancelCommand::isRunnable(const CursorContext &payload) {
    return payload.focus_target == FocusTarget::Prompt;
}
