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
#include "PromptCommand.h"


PromptCommand::PromptCommand(Prompt &prompt, PromptState &promptState)
    : m_prompt(prompt),
      m_prompt_state(promptState) {}

void PromptCommand::provideAutoComplete(const std::span<const std::u16string_view> previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) const {
    (void) previousArgs;
    (void) input;
    if (argumentIndex == 0) {
        itemCallback(u"confirm");
        itemCallback(u"cancel");
    }
}

std::optional<std::u16string> PromptCommand::run(CursorContext &payload, const std::span<const std::u16string_view> args) {
    if (args.size() != 1 || (args[0] != u"confirm" && args[0] != u"cancel")) {
        return u"Usage: prompt <confirm|cancel>";
    }

    // Only a running prompt (typed command line, or pending feedback question) can be driven.
    if (m_prompt_state.getRunningState() != PromptState::RunningState::Running) {
        return std::nullopt;
    }

    // A confirm may run "buffer close", destroying `payload`: flag the redraw first and
    // never touch the payload after the delegation.
    payload.wants_redraw = true;
    if (args[0] == u"confirm") {
        m_prompt.confirm(payload, m_prompt_state);
    } else {
        m_prompt.cancel(payload, m_prompt_state);
    }

    return std::nullopt;
}
