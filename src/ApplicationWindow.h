#ifndef APPLICATION_WINDOW_H
#define APPLICATION_WINDOW_H

#include <array>
#include <string_view>

#include <SDL.h>

#include "core/command/cvar/CVarFloat.h"
#include "core/command/CommandManager.h"
#include "core/cursor/PromptCursor.h"
#include "core/cursor/Cursor.h"
#include "core/renderer/QuadBuffer.h"
#include "core/renderer/QuadProgram.h"
#include "core/theme/Theme.h"
#include "editor/Editor.h"
#include "editor/EditorState.h"
#include "infobar/InfoBar.h"
#include "infobar/InfoBarState.h"
#include "prompt/Prompt.h"
#include "prompt/PromptState.h"


/**
 * @brief Main application window that manages rendering, input events, and UI components.
 *
 * Responsible for SDL window and OpenGL context lifecycle, initializing core subsystems,
 * managing view layout and redraw state, and running the application's main event loop.
 */
class ApplicationWindow final {
private:
    /** Maximum number of renderable quads in the geometry buffer. */
    static constexpr auto MAX_QUADS = 8192;

    /** @brief Represents the currently focused input target. */
    enum class FocusTarget {
        Editor, ///< Editor view is focused.
        Prompt  ///< Prompt view is focused.
    };

    /** @brief Dirty flags used for tracking pending updates. */
    enum DirtyFlag {
        Views  = 1,           ///< UI views need to be redrawn.
        Matrix = Views << 1   ///< Projection matrix needs updating.
    };

private:
    /** SDL window handle. */
    SDL_Window* p_sdl_window;

    /** OpenGL rendering context. */
    SDL_GLContext m_sdl_gl_context;

    /** Command system manager (commands, CVars, history, etc.). */
    CommandManager m_command_manager;

    /** Theme manager for fonts, colors, and UI style. */
    Theme m_theme;

    /** Syntax highlighter used by views. */
    HighLighter m_high_lighter;

    /** Shader program used to render textured quads. */
    QuadProgram m_quad_program;

    /** Geometry buffer for batched quad rendering. */
    QuadBuffer m_quad_buffer;

    /** CVar tracking the maximum frame time (to render, before swapping). */
    std::shared_ptr<CVarFloat> m_render_time;

    /** The prompt cursor. */
    PromptCursor m_prompt_cursor;

    /** The only cursor for now */
    Cursor m_cursor;

    /** Top info bar view. */
    InfoBar m_info_bar;

    /** Main editor view. */
    Editor m_editor;

    /** State tracking the info bar. */
    InfoBarState m_info_bar_state;

    /** State object tracking the editor. */
    EditorState m_editor_state;

    /** State object tracking the prompt. */
    PromptState m_prompt_state;

    /** Bottom command prompt view. */
    Prompt m_prompt;

    /** 4x4 orthogonal projection matrix for 2D rendering. */
    std::array<float, 16> m_orthogonal;

    /**
     * @brief Recomputes the orthogonal projection matrix.
     * @param width New window width.
     * @param height New window height.
     */
    void updateOrthogonal(int32_t width, int32_t height);

    /** @brief Registers the built-in ":open" command. */
    void registerOpenCommand();

    /** @brief Registers the built-in ":save" command. */
    void registerSaveCommand();

    /** @brief Registers the built-in render time command and cvar. */
    void registerRenderTimeCommand();

    /** @brief Registers the quit command. */
    void registerQuitCommand();

public:
    /** @brief Deleted copy constructor. */
    ApplicationWindow(const ApplicationWindow &) = delete;

    /** @brief Deleted copy assignment operator. */
    ApplicationWindow &operator=(const ApplicationWindow &) = delete;

    /** @brief Constructs the ApplicationWindow with default values. */
    explicit ApplicationWindow();

    /**
     * @brief Creates the SDL window and initializes OpenGL context.
     * @param title The window title.
     * @param width Initial window width in pixels.
     * @param height Initial window height in pixels.
     */
    void create(std::string_view title, int32_t width, int32_t height);

    /** @brief Cleans up all allocated resources and destroys the window. */
    void destroy();

    /** @brief Starts the main application loop (event handling and rendering). */
    void mainLoop();
};


#endif //APPLICATION_WINDOW_H
