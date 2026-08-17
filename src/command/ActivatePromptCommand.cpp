/*
* Copyright (C) 2026 Romain Graillot
 *
 * This file is part of bbloc.
 *
 * bbloc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bbloc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "ActivatePromptCommand.h"


ActivatePromptCommand::ActivatePromptCommand(Prompt &prompt, PromptState &promptState)
    : m_prompt(prompt),
      m_prompt_state(promptState) {}

void ActivatePromptCommand::provideAutoComplete(const std::span<const std::u16string_view> previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) previousArgs;
    (void) input;
    (void) argumentIndex;
    (void) itemCallback;
    // No-op
}

std::optional<std::u16string> ActivatePromptCommand::run(CursorContext &payload, const std::span<const std::u16string_view> args) {
    if (!args.empty()) {
        return u"Expected 0 argument.";
    }

    // A prompt already taking input is validated rather than restarted: the bound key opens the
    // prompt, then submits what was typed, which is the only way a controller reaches Return.
    // A confirm may run "buffer close", destroying `payload`: flag the redraw first and never
    // touch the payload after the delegation.
    if (m_prompt_state.getRunningState() == PromptState::RunningState::Running) {
        payload.wants_redraw = true;
        m_prompt.confirm(payload, m_prompt_state);
        return std::nullopt;
    }

    // Set prompt to running state
    m_prompt_state.setRunningState(PromptState::RunningState::Running);
    m_prompt_state.setPromptText(PromptState::PROMPT_ACTIVE);

    // Clear the user text and set the focus to the prompt (since the editor had it if we run from a binding)
    payload.prompt_cursor.clear();
    payload.focus_target = FocusTarget::Prompt;
    payload.search.resetMatches();
    payload.wants_redraw = true;

    return std::nullopt;
}
