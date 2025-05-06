#ifndef INFO_BAR_H
#define INFO_BAR_H


#include "../core/command/CommandManager.h"
#include "../core/cursor/Cursor.h"
#include "../core/renderer/QuadProgram.h"
#include "../core/renderer/QuadBuffer.h"
#include "../core/theme/Theme.h"
#include "../core/View.h"
#include "InfoBarState.h"


/**
 * @brief The InfoBar view, displayed at the top of the screen.
 *
 * This view renders contextual information like:
 * - Current file name
 * - Cursor position (line and column)
 * - Current highlight mode
 * - ect.
 */
class InfoBar final : public View<Cursor, InfoBarState> {
private:
    /**
     * @brief: Draw the background layer of the info bar.
     * @param viewState A reference to the InfoBar view state.
     */
    void drawBackground(const InfoBarState& viewState) const;

    /**
      * @brief: Draw the text layer of the info bar.
      * @param highLighter A reference to the HighLighter.
      * @param cursor A reference to the PromptCursor.
      * @param viewState A reference to the Prompt view state.
      */
    void drawText(const HighLighter& highLighter, const Cursor& cursor, const InfoBarState& viewState) const;

public:
    /**
     * @brief Constructs the InfoBar view.
     *
     * @param commandManager Reference to the command manager.
     * @param theme Reference to the Theme (fonts, colors, etc.).
     * @param quadProgram Reference to the quad shader program.
     * @param quadBuffer Reference to the quad buffer.
     */
    explicit InfoBar(CommandManager& commandManager, Theme& theme, QuadProgram& quadProgram, QuadBuffer& quadBuffer);

    /**
     * @brief Renders the InfoBar.
     * @param highLighter Unused in InfoBar but required by base class.
     * @param cursor Reference to the current cursor.
     * @param viewState The InfoBarState of this view.
     * @param dt Time delta since the last frame.
     */
    void render(const HighLighter& highLighter, const Cursor& cursor, InfoBarState& viewState, float dt) override;

    /**
     * @brief InfoBar does not handle key input.
     * @return Always returns false.
     */
    bool onKeyDown(const HighLighter& highLighter, Cursor& cursor, InfoBarState& viewState, SDL_Keycode keyCode, uint16_t keyModifier) const override;

    /** @brief InfoBar does not handle text input. */
    void onTextInput(const HighLighter& highLighter, Cursor& cursor, InfoBarState& viewState, const char* text) const override;
};


#endif //INFO_BAR_H
