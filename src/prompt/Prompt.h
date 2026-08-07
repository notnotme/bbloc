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
#ifndef PROMPT_H
#define PROMPT_H

#include <SDL.h>

#include "../core/renderer/QuadProgram.h"
#include "../core/renderer/QuadBuffer.h"
#include "../core/theme/Theme.h"
#include "../core/View.h"
#include "PromptState.h"


/**
 * @brief Represents the interactive command prompt displayed at the bottom of the screen.
 *
 * Handles rendering of the prompt input line, managing command execution,
 * and offering auto-completions.
 */
class Prompt final : public View<PromptState> {
private:
    /** @brief Quads reserved in the staging vector when this view begins its batch; advisory only. */
    static constexpr uint32_t DEFAULT_QUAD_COUNT = 1024;

    /**
     * @brief: Draw the background layer of the prompt.
     *
     * @param quadBuffer A reference to the quad buffer receiving the quads.
     * @param viewState A reference to the Prompt view state.
     */
    void drawBackground(QuadBuffer &quadBuffer, const PromptState &viewState) const;

    /**
     * @brief: Draw the text layer of the prompt.
     *
     * @param quadBuffer A reference to the quad buffer receiving the quads.
     * @param context A reference to the cursor context.
     * @param viewState A reference to the Prompt view state.
     */
    void drawText(QuadBuffer &quadBuffer, const CursorContext &context, const PromptState &viewState) const;

public:
    /**
     * @brief Constructs a Prompt view instance.
     *
     * @param commandController Reference to the CommandManager instance.
     * @param theme Reference to the Theme manager for styling.
     * @param quadProgram Reference to the QuadProgram for rendering.
     */
    explicit Prompt(GlobalRegistry<CursorContext> &commandController, Theme &theme, QuadProgram &quadProgram);

    /**
     * @brief Renders the command prompt on screen.
     *
     * @param context Reference to the cursor context.
     * @param viewState The associated PromptState for layout/input data.
     * @param quadBuffer Reference to the quad buffer used to build this frame's geometry.
     * @param dt Time delta since the last frame.
     */
    void render(CursorContext &context, PromptState &viewState, QuadBuffer &quadBuffer, float dt) override;

    /**
     * @brief Handles key events while the prompt is active.
     *
     * Return and Escape delegate to confirm() and cancel().
     *
     * @param context Reference to the cursor context.
     * @param viewState The prompt's view state.
     * @param keyCode SDL key code.
     * @param keyModifier Key modifier mask.
     * @return True if input was handled.
     */
    bool onKeyDown(CursorContext &context, PromptState &viewState, SDL_Keycode keyCode, uint16_t keyModifier) const override;

    /**
     * @brief Validates the prompt line: dispatches the typed command, or answers a pending feedback.
     *
     * The dispatched command may close the active buffer and destroy `context`; callers must
     * not touch it after this returns.
     *
     * @param context Reference to the cursor context.
     * @param viewState The prompt's view state.
     */
    void confirm(CursorContext &context, PromptState &viewState) const;

    /**
     * @brief Cancels the prompt: resets the line, drops any pending feedback, and focuses the editor.
     *
     * @param context Reference to the cursor context.
     * @param viewState The prompt's view state.
     */
    void cancel(CursorContext &context, PromptState &viewState) const;

    /**
     * @brief Handles raw text input for the prompt.
     *
     * @param context Reference to the cursor context.
     * @param viewState The prompt's view state.
     * @param text UTF-8 encoded character input.
     */
    void onTextInput(CursorContext &context, PromptState &viewState, const char* text) const override;
};


#endif //PROMPT_H
