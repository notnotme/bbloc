#ifndef PROMPT_H
#define PROMPT_H

#include <SDL.h>

#include "../core/cursor/PromptCursor.h"
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
    /**
     * @brief: Draw the background layer of the prompt.
     * @param viewState A reference to the Prompt view state.
     */
    void drawBackground(const PromptState &viewState) const;

    /**
     * @brief: Draw the text layer of the prompt.
     * @param context A reference to the cursor context.
     * @param viewState A reference to the Prompt view state.
     */
    void drawText(const CursorContext &context, const PromptState &viewState) const;

public:
    /**
     * @brief Constructs a Prompt view instance.
     * @param commandController Reference to the CommandManager instance.
     * @param theme Reference to the Theme manager for styling.
     * @param quadProgram Reference to the QuadProgram for rendering.
     * @param quadBuffer Reference to the QuadBuffer for geometry submission.
     */
    explicit Prompt(CommandController<CursorContext> &commandController, Theme &theme, QuadProgram &quadProgram, QuadBuffer &quadBuffer);

    /**
     * @brief Renders the command prompt on screen.
     * @param context Reference to the cursor context.
     * @param viewState The associated PromptState for layout/input data.
     * @param dt Time delta since the last frame.
     */
    void render(CursorContext &context, PromptState &viewState, float dt) override;

    /**
     * @brief Handles key events while the prompt is active.
     * @param context Reference to the cursor context.
     * @param viewState The prompt's view state.
     * @param keyCode SDL key code.
     * @param keyModifier Key modifier mask.
     * @return True if input was handled.
     */
    bool onKeyDown(CursorContext &context, PromptState &viewState, SDL_Keycode keyCode, uint16_t keyModifier) const override;

    /**
     * @brief Handles raw text input for the prompt.
     * @param context Reference to the cursor context.
     * @param viewState The prompt's view state.
     * @param text UTF-8 encoded character input.
     */
    void onTextInput(CursorContext &context, PromptState &viewState, const char* text) const override;
};


#endif //PROMPT_H
