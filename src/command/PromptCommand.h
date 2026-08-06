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
#ifndef PROMPT_COMMAND_H
#define PROMPT_COMMAND_H

#include <span>
#include <string>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"
#include "../prompt/Prompt.h"
#include "../prompt/PromptState.h"


/**
 * @brief Command driving the active prompt interaction, mirroring the Return and Escape keys.
 *
 * Meant for controller bindings: "prompt confirm" validates the prompt line and
 * "prompt cancel" dismisses it, through the same Prompt entry points the keyboard uses.
 * The command is a no-op while the prompt is not running, so the bound buttons are safe to
 * press at any time; a running prompt covers feedback questions too.
 */
class PromptCommand final : public Command<CursorContext> {
private:
    /** Reference to the prompt view the actions delegate to. */
    Prompt &m_prompt;

    /** Reference to the prompt state, gating the actions on the Running state. */
    PromptState &m_prompt_state;

public:
    /**
     * @brief Constructs a PromptCommand with the prompt view and state it drives.
     *
     * @param prompt Reference to the prompt view.
     * @param promptState Reference to the prompt state.
     */
    explicit PromptCommand(Prompt &prompt, PromptState &promptState);

    /**
     * @brief Provides auto-completion suggestions for command arguments.
     *
     * Suggests the two actions for the first argument.
     *
     * @param previousArgs The arguments typed before the one being completed, excluding the command name.
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideAutoComplete(std::span<const std::u16string_view> previousArgs, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) const override;

    /**
     * @brief Executes the prompt action.
     *
     * Expects one argument, "confirm" or "cancel". A confirm may destroy the payload context
     * (e.g. the prompt line runs "buffer close"), so the redraw is flagged before delegating
     * and the payload is never touched afterwards.
     *
     * @param payload The cursor context the prompt interaction applies to.
     * @param args Command arguments. One action argument is expected.
     * @return An optional message indicating the result of the operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, std::span<const std::u16string_view> args) override;
};


#endif //PROMPT_COMMAND_H
