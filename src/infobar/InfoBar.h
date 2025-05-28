#ifndef INFO_BAR_H
#define INFO_BAR_H


#include "../core/base/GlobalRegistry.h"
#include "../core/renderer/QuadProgram.h"
#include "../core/renderer/QuadBuffer.h"
#include "../core/theme/Theme.h"
#include "../core/View.h"
#include "../core/ViewState.h"
#include "../core/CursorContext.h"


/**
 * @brief The InfoBar view, displayed at the top of the screen.
 *
 * This view renders contextual information like:
 * - Current file name
 * - Cursor position (line and column)
 * - Current highlight mode
 * - ect.
 */
class InfoBar final : public View<> {
private:
    /**
     * @brief: Draw the background layer of the info bar.
     *
     * @param viewState A reference to the InfoBar view state.
     */
    void drawBackground(const ViewState &viewState) const;

    /**
      * @brief: Draw the text layer of the info bar.
      *
      * @param context A reference to the cursor context.
      * @param viewState A reference to the Prompt view state.
      */
    void drawText(const CursorContext &context, const ViewState &viewState) const;

public:
    /**
     * @brief Constructs the InfoBar view.
     *
     * @param commandController Reference to the command controller.
     * @param theme Reference to the Theme (fonts, colors, etc.).
     * @param quadProgram Reference to the quad shader program.
     * @param quadBuffer Reference to the quad buffer.
     */
    explicit InfoBar(GlobalRegistry<CursorContext> &commandController, Theme &theme, QuadProgram &quadProgram, QuadBuffer &quadBuffer);

    /**
     * @brief Renders the InfoBar.
     *
     * @param context Reference to the cursor context.
     * @param viewState The InfoBarState of this view.
     * @param dt Time delta since the last frame.
     */
    void render(CursorContext &context, ViewState &viewState, float dt) override;

    /**
     * @brief InfoBar does not handle key input.
     *
     * @return Always returns false.
     */
    bool onKeyDown(CursorContext &context, ViewState &viewState, SDL_Keycode keyCode, uint16_t keyModifier) const override;

    /** @brief InfoBar does not handle text input. */
    void onTextInput(CursorContext &context, ViewState &viewState, const char* text) const override;
};


#endif //INFO_BAR_H
