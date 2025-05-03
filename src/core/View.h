#ifndef VIEW_H
#define VIEW_H

#include <SDL.h>

#include "highlight/HighLighter.h"
#include "renderer/QuadProgram.h"
#include "renderer/QuadBuffer.h"
#include "theme/Theme.h"
#include "ViewState.h"


/**
 * @brief Base class for rendering a view in the application (e.g., editor, console).
 *
 * This class defines the interface and common rendering infrastructure shared by different views.
 * It is templated on a cursor type (`TCursor`) and an optional view state (`TState`, defaults to `ViewState`).
 *
 * Derived classes must implement specific behaviors like input handling and rendering.
 *
 * @tparam TCursor Type of the cursor used for text navigation/editing.
 * @tparam TState State type for additional view-specific information.
 */
template <class TCursor, class TState = ViewState>
class View {
protected:
    /** Reference to the command manager (used for dynamic runtime controls). */
    CommandManager& m_command_manager;

    /** Reference to the theme used for rendering (colors, fonts, etc.). */
    Theme& m_theme;

    /** Reference to the shader program for quad rendering. */
    QuadProgram& m_quad_program;

    /** Reference to the quad buffer used to build the glyph buffer. */
    QuadBuffer& m_quad_buffer;

    /** Current window width in pixels. */
    int32_t m_window_width;

    /** Current window height in pixels. */
    int32_t m_window_height;

public:
    /** @brief Deleted copy constructor. */
    View(const View &) = delete;

    /** @brief Deleted copy assignment operator. */
    View &operator=(const View &) = delete;

    /** @brief For inheritance */
    virtual ~View() = default;

    /**
     * @brief Constructs a view with references to rendering and theme resources.
     * @param commandManager Reference to the command manager.
     * @param theme Reference to the theme manager.
     * @param quadProgram Reference to the quad shader program.
     * @param quadBuffer Reference to the quad geometry buffer.
     */
    explicit View(CommandManager& commandManager, Theme& theme, QuadProgram& quadProgram, QuadBuffer& quadBuffer);

    /** @brief Initializes the view and its resources. Must be implemented by derived class. */
    virtual void create() = 0;

    /** @brief Cleans up view resources. Must be implemented by derived class. */
    virtual void destroy() = 0;


    /**
     * @brief Renders the view contents.
     * @param highLighter Reference to the highlighter for syntax highlighting.
     * @param cursor Reference to the cursor representing the current text position.
     * @param viewState Reference to the view-specific state.
     * @param dt Delta time in seconds (useful for animations or transitions).
     */
    virtual void render(const HighLighter& highLighter, const TCursor& cursor, TState& viewState, float dt) = 0;

    /**
     * @brief Handles key press events.
     * @param highLighter Reference to the highlighter.
     * @param cursor Reference to the cursor.
     * @param viewState Reference to the view state.
     * @param keyCode The SDL key code.
     * @param keyModifier Bitmask of modifier keys (CTRL, ALT, etc.).
     * @return true if the key was handled, false otherwise.
     */
    virtual bool onKeyDown(const HighLighter& highLighter, TCursor& cursor, TState& viewState, SDL_Keycode keyCode, uint16_t keyModifier) const = 0;

    /**
     * @brief Handles UTF-8 text input events (e.g., from typing).
     * @param highLighter Reference to the highlighter.
     * @param cursor Reference to the cursor.
     * @param viewState Reference to the view state.
     * @param text UTF-8 encoded input text (from SDL_TEXTINPUT).
     */
    virtual void onTextInput(const HighLighter& highLighter, TCursor& cursor, TState& viewState, const char* text) const = 0;

    /**
     * @brief Updates the internal window size for the view (e.g., after a resize event).
     * @param width New window width in pixels.
     * @param height New window height in pixels.
     */
    void resizeWindow(int32_t width, int32_t height);
};

template<class TCursor, class TState>
View<TCursor, TState>::View(CommandManager &commandManager, Theme &theme, QuadProgram &quadProgram, QuadBuffer &quadBuffer)
    : m_command_manager(commandManager),
      m_theme(theme),
      m_quad_program(quadProgram),
      m_quad_buffer(quadBuffer),
      m_window_width(0),
      m_window_height(0) {}

template<class TCursor, class TState>
void View<TCursor, TState>::resizeWindow(int32_t width, int32_t height) {
    m_window_width = width;
    m_window_height = height;
}


#endif //VIEW_H
