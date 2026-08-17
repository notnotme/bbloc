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
#ifndef ACTIVATE_PROMPT_COMMAND_H
#define ACTIVATE_PROMPT_COMMAND_H

#include <span>
#include <string>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/CursorContext.h"
#include "../core/base/Command.h"
#include "../prompt/Prompt.h"
#include "../prompt/PromptState.h"


/**
 * @brief Command that activates the prompt functionality in the text editor.
 *
 * This class implements the Command interface for activating and managing
 * the prompt within the editor. It handles the interaction between user input
 * and the prompt state, providing auto-completion suggestions and validating
 * when the command can be executed.
 *
 * Running it on an already running prompt validates the line instead of restarting it, so the
 * bound key opens the prompt and then submits it. This is what makes the prompt usable from a
 * controller with one button: the pad has no Return key, and the alternative — clearing a line
 * the user just typed — is never what pressing the open key twice was meant to do.
 */
class ActivatePromptCommand final : public Command<CursorContext> {
private:
    /** Reference to the prompt view a second activation validates through. */
    Prompt &m_prompt;

    /** Reference to the prompt state this command will manipulate. */
    PromptState &m_prompt_state;

public:
    /**
     * @brief Constructs an ActivatePromptCommand with the given prompt view and state.
     *
     * @param prompt Reference to the prompt view a second activation confirms through.
     * @param promptState Reference to the prompt state to be activated and managed.
     */
    explicit ActivatePromptCommand(Prompt &prompt, PromptState &promptState);

    /**
     * @brief Provides auto-completion suggestions for command arguments.
     *
     * This command does not expect any completion.
    *
     * @param previousArgs The arguments typed before the one being completed, excluding the command name.
     * @param argumentIndex The index of the argument currently being completed.
     * @param input The current partial input from the user for this argument.
     * @param itemCallback A callback to be invoked with each completion suggestion.
     */
    void provideAutoComplete(std::span<const std::u16string_view> previousArgs, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) const override;

    /**
     * @brief Executes the prompt activation command.
     *
     * Activates the prompt with the given context, or validates it when it is already running.
     * A validation may destroy the payload context (e.g. the prompt line runs "buffer close"),
     * so the redraw is flagged before delegating and the payload is never touched afterwards.
     * The args vector must be empty.
     *
     * @param payload The cursor context to use when activating the prompt.
     * @param args Command arguments. An empty vector is expected.
     * @return An optional message indicating the result of the operation.
     */
    [[nodiscard]] std::optional<std::u16string> run(CursorContext &payload, std::span<const std::u16string_view> args) override;
};


#endif //ACTIVATE_PROMPT_COMMAND_H
