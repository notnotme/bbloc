#ifndef EDITOR_H
#define EDITOR_H

#include <SDL.h>

#include "../core/base/GlobalRegistry.h"
#include "../core/cvar/CVarBool.h"
#include "../core/renderer/QuadProgram.h"
#include "../core/renderer/QuadBuffer.h"
#include "../core/theme/Theme.h"
#include "../core/View.h"
#include "../core/ViewState.h"
#include "../core/CursorContext.h"


/**
 * @brief Main text editor view responsible for rendering text and handling input.
 *
 * The Editor view manages the rendering the cursor buffer, processing user input,
 * tracking scroll state, everything via CursorContext. It also tries to optimize layout with a longest-line cache.
 */
class Editor final : public View<> {
private:
    /**
     * @brief Internal structure used to cache the longest visible line.
     *
     * Optimizes horizontal scroll range and avoids remeasuring the longest line every frame.
     */
    struct LongestLineCache {
        uint32_t index; ///< Line index of the longest line.
        uint32_t count; ///< Character count in the longest line.
        int32_t width;  ///< Width in pixels of the longest line.
    };

    /** Cache used for optimizing horizontal scroll and layout. */
    LongestLineCache m_longest_line_cache;

    /** CVar for toggling tab-to-space replacement in input. */
    std::shared_ptr<CVarBool> m_is_tab_to_space;

    /** @brief Registers the tab_to_space cvar into the command manager. */
    void registerTabToSpaceCVar() const;

    /**
     * @brief Recomputes the longest line cache.
     *
     * @param context Reference to the cursor context.
     */
    void updateLongestLineCache(const CursorContext &context);

    /**
     * @brief: Compute scroll position and max scroll for the horizontal and vertical axis.
     *
     * @param context A reference to the cursor context.
     * @param viewState A reference to the Editor view state.
     */
    void updateScroll(CursorContext &context, const ViewState &viewState) const;

    /**
     * @brief: Draw the background layer of the editor.
     *
     * @param viewState A reference to the Editor view state.
     * @param marginWidth The width of the margin, without the border size.
     */
    void drawBackground(const ViewState &viewState, int32_t marginWidth) const;

    /**
     * @brief: Draw the text layer in the left margin of the editor.
     *
     * @param context A reference to the cursor context.
     * @param viewState A reference to the Editor view state.
     * @param lineCountWidth The width in pixel of the greatest line number.
     * @param scrollY The editor y scroll offset.
     */
    void drawMarginText(const CursorContext &context, const ViewState &viewState, int32_t lineCountWidth, int32_t scrollY) const;

    /**
     * @brief: Draw the Cursor the text layer in the of the editor.
     *
     * @param context A reference to the cursor context.
     * @param viewState A reference to the Editor view state.
     * @param scrollX The editor x scroll offset.
     * @param scrollY The editor y scroll offset.
     */
    void drawText(const CursorContext &context, const ViewState &viewState, int32_t scrollX, int32_t scrollY) const;

public:
    /**
     * @brief Constructs the Editor view.
     *
     * @param commandController Reference to the CommandController.
     * @param theme Reference to the Theme for rendering.
     * @param quadProgram Reference to the QuadProgram shader.
     * @param quadBuffer Reference to the geometry buffer.
     */
    explicit Editor(GlobalRegistry<CursorContext> &commandController, Theme &theme, QuadProgram &quadProgram, QuadBuffer &quadBuffer);

    /**
     * @brief Renders the text editor to the screen.
     *
     * @param context Reference to the cursor context.
     * @param viewState State of the editor view.
     * @param dt Time delta since the last frame.
     */
    void render(CursorContext &context, ViewState &viewState, float dt) override;

    /**
     * @brief Handles key down events in the editor.
     *
     * @param context Reference to the cursor context.
     * @param viewState State of the editor view.
     * @param keyCode SDL key code.
     * @param keyModifier Modifier bitmask (Shift, Ctrl, etc).
     * @return True if the event was handled.
     */
    bool onKeyDown(CursorContext &context, ViewState &viewState, SDL_Keycode keyCode, uint16_t keyModifier) const override;

    /**
     * @brief Handles text input events in the editor.
     *
     * @param context Reference to the cursor context.
     * @param viewState State of the editor view.
     * @param text UTF-8 encoded character input from SDL_TEXTINPUT.
     */
    void onTextInput(CursorContext &context, ViewState &viewState, const char* text) const override;
};


#endif //EDITOR_H
