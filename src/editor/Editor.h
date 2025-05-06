#ifndef EDITOR_H
#define EDITOR_H

#include <SDL.h>

#include "../core/command/cvar/CVarBool.h"
#include "../core/command/CommandManager.h"
#include "../core/cursor/Cursor.h"
#include "../core/highlight/HighLighter.h"
#include "../core/renderer/QuadProgram.h"
#include "../core/renderer/QuadBuffer.h"
#include "../core/theme/Theme.h"
#include "../core/View.h"
#include "EditorState.h"


/**
 * @brief Main text editor view responsible for rendering text and handling input.
 *
 * The Editor view manages the rendering the cursor buffer, processing user input,
 * tracking scroll state via EditorState, and tries to optimize layout with a longest-line cache.
 */
class Editor final : public View<Cursor, EditorState> {
private:
    /**
     * @brief Internal structure used to cache the longest visible line.
     * Optimizes horizontal scroll range and avoids remeasuring the longest line every frame.
     */
    struct LongestLineCache {
        uint32_t index; ///< Line index of the longest line.
        uint32_t count; ///< Character count in the longest line.
        int32_t width; ///< Width in pixels of the longest line.
    };

    /** Cache used for optimizing horizontal scroll and layout. */
    LongestLineCache m_longest_line_cache;

    /** CVar for toggling tab-to-space replacement in input. */
    std::shared_ptr<CVarBool> m_is_tab_to_space;

    /** @brief Registers the tab_to_space cvar into the command manager. */
    void registerTabToSpaceCVar() const;

    /**
     * @brief Recomputes the longest line cache.
     * @param cursor Reference to the text cursor buffer.
     */
    void updateLongestLineCache(const Cursor& cursor);

    /**
     * @brief: Compute scroll position and max scroll for the horizontal and vertical axis.
     * @param cursor A reference to the Cursor.
     * @param viewState A reference to the Editor view state.
     */
    void updateScroll(const Cursor& cursor, EditorState& viewState) const;

    /**
     * @brief: Draw the background layer of the editor.
     * @param viewState A reference to the Editor view state.
     * @param marginWidth The width of the margin, without the border size.
     */
    void drawBackground(const EditorState& viewState, int32_t marginWidth) const;

    /**
     * @brief: Draw the text layer in the left margin of the editor.
     * @param cursor A reference to the Cursor.
     * @param viewState A reference to the Editor view state.
     * @param lineCountWidth The width in pixel of the greatest line number.
     * @param scrollY The editor y scroll offset.
     */
    void drawMarginText(const Cursor& cursor, const EditorState& viewState, int32_t lineCountWidth, int32_t scrollY) const;

    /**
     * @brief: Draw the Cursor the text layer in the of the editor.
     * @param highLighter A reference to the HighLighter.
     * @param cursor A reference to the Cursor.
     * @param viewState A reference to the Editor view state.
     * @param scrollX The editor x scroll offset.
     * @param scrollY The editor y scroll offset.
     */
    void drawText(const HighLighter& highLighter, const Cursor& cursor, const EditorState& viewState, int32_t scrollX, int32_t scrollY);

public:
    /**
     * @brief Constructs the Editor view.
     * @param commandManager Reference to the CommandManager.
     * @param theme Reference to the Theme for rendering.
     * @param quadProgram Reference to the QuadProgram shader.
     * @param quadBuffer Reference to the geometry buffer.
     */
    explicit Editor(CommandManager& commandManager, Theme& theme, QuadProgram& quadProgram, QuadBuffer& quadBuffer);

    /**
     * @brief Renders the text editor to the screen.
     * @param highLighter Reference to the syntax highlighter.
     * @param cursor Reference to the text cursor.
     * @param viewState State of the editor view.
     * @param dt Time delta since the last frame.
     */
    void render(const HighLighter& highLighter, const Cursor& cursor, EditorState& viewState, float dt) override;

    /**
     * @brief Handles key down events in the editor.
     * @param highLighter Reference to the syntax highlighter.
     * @param cursor Reference to the text cursor.
     * @param viewState State of the editor view.
     * @param keyCode SDL key code.
     * @param keyModifier Modifier bitmask (Shift, Ctrl, etc).
     * @return True if the event was handled.
     */
    bool onKeyDown(const HighLighter& highLighter, Cursor& cursor, EditorState& viewState, SDL_Keycode keyCode, uint16_t keyModifier) const override;

    /**
     * @brief Handles text input events in the editor.
     * @param highLighter Reference to the syntax highlighter.
     * @param cursor Reference to the text cursor.
     * @param viewState State of the editor view.
     * @param text UTF-8 encoded character input from SDL_TEXTINPUT.
     */
    void onTextInput(const HighLighter& highLighter, Cursor& cursor, EditorState& viewState, const char* text) const override;
};


#endif //EDITOR_H
