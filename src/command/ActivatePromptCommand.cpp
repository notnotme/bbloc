#include "ActivatePromptCommand.h"

ActivatePromptCommand::ActivatePromptCommand(PromptState &promptState)
    : m_prompt_state(promptState) {}

void ActivatePromptCommand::provideAutoComplete(const int32_t argumentIndex, const std::string_view input, const AutoCompleteCallback<char> &itemCallback) const {
    (void) input;
    (void) argumentIndex;
    (void) itemCallback;
    // No-op
}

std::optional<std::u16string> ActivatePromptCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    if (!args.empty()) {
        return u"Expected 0 argument.";
    }

    // Set prompt to running state
    m_prompt_state.setRunningState(PromptState::RunningState::Running);
    m_prompt_state.setPromptText(PromptState::PROMPT_ACTIVE);

    // Clear the user text and set the focus to the prompt (since the editor had it if we run from a binding)
    payload.prompt_cursor.clear();
    payload.focus_target = FocusTarget::Prompt;
    payload.wants_redraw = true;

    return std::nullopt;
}

bool ActivatePromptCommand::isRunnable(const CursorContext &payload) {
    // Only the editor focused can trigger this command
    return payload.focus_target == FocusTarget::Editor;
}
