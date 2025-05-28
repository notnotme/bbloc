#include "ValidateCommand.h"


ValidateCommand::ValidateCommand(PromptState &promptState)
    : m_prompt_state(promptState) {}

void ValidateCommand::provideAutoComplete(const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) input;
    (void) argumentIndex;
    (void) itemCallback;
    // No-op
}

std::optional<std::u16string> ValidateCommand::run(CursorContext &payload, const std::vector<std::u16string_view> &args) {
    if (!args.empty()) {
        return u"Expected 0 argument.";
    }

    // runCommand can also update the prompt state, set the prompt to Idle before running the command.
    m_prompt_state.setRunningState(PromptState::RunningState::Idle);

    // The return value of runCommand can be ignored in this use case,
    // and because we just check the current focus inside Command::isRunnable to block commands, in facts.
    const auto prompt_command = payload.prompt_cursor.getString();
    payload.command_runner.runCommand(prompt_command, true);

    return std::nullopt;
}

bool ValidateCommand::isRunnable(const CursorContext &payload) {
    return payload.focus_target == FocusTarget::Prompt;
}
