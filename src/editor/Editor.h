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
 * The Editor view manages rendering the cursor buffer, processing user input,
 * tracking scroll state via EditorState, and try optimizing layout with a longest-line cache.
 */
class Editor final : public View<Cursor, EditorState> {
private:
    /**
     * @brief Internal structure used to cache longest visible line.
     * Optimizes horizontal scroll range and avoids remeasuring the longest line every frame.
     */
    struct LongestLineCache {
        int32_t index; ///< Line index of the longest line.
        int32_t width; ///< Width in pixels of the longest line.
        int32_t count; ///< Character count in the longest line.
    };

    /** Cache used for optimizing horizontal scroll and layout. */
    LongestLineCache m_longest_line_cache;

    /** CVar for toggling tab-to-space replacement in input. */
    std::shared_ptr<CVarBool> m_is_tab_to_space;

    /**
     * @brief Recomputes the longest line cache.
     * @param cursor Reference to the text cursor buffer.
     * @param cursorLineCount Total number of lines.
     * @param cursorLine Current cursor line index.
     * @param cursorString Text content of the current line.
     */
    void updateLongestLineCache(const Cursor& cursor, int32_t cursorLineCount, int32_t cursorLine, std::u16string_view cursorString);

    /** @brief Registers editor-specific CVars with the command system. */
    void registerCVar() const;

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
