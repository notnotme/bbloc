#ifndef APPLICATION_WINDOW_H
#define APPLICATION_WINDOW_H

#include <array>
#include <string_view>

#include <SDL.h>

#include "core/cvar/CVarFloat.h"
#include "core/CommandManager.h"
#include "core/cursor/PromptCursor.h"
#include "core/renderer/QuadBuffer.h"
#include "core/renderer/QuadProgram.h"
#include "core/theme/Theme.h"
#include "core/CursorContext.h"
#include "command/BindCommand.h"
#include "editor/Editor.h"
#include "infobar/InfoBar.h"
#include "prompt/Prompt.h"
#include "prompt/PromptState.h"


/**
 * @brief Main application window that manages rendering, input events, and UI components.
 *
 * Responsible for SDL window and OpenGL context lifecycle, initializing core subsystems,
 * managing view layout and redraw state, and running the application's main event loop.
 */
class ApplicationWindow final : public CommandRunner {
public:
    /** Maximum number of renderable quads in m_quad_buffer. */
    static constexpr auto MAX_QUADS = 8192;

    /** Start offset of quads in m_quad_buffer in the info bar view */
    static constexpr auto INFO_BAR_BUFFER_QUAD_OFFSET = 0;

    /** Maximum quads available for the info bar view */
    static constexpr auto INFO_BAR_BUFFER_QUAD_COUNT = 1024;

    /** Start offset of quads in m_quad_buffer in the prompt view */
    static constexpr auto PROMPT_BUFFER_QUAD_OFFSET = INFO_BAR_BUFFER_QUAD_COUNT;

    /** Maximum quads available for the prompt view */
    static constexpr auto PROMPT_BUFFER_QUAD_COUNT = 1024;

    /** Start offset of quads in m_quad_buffer in the editor view */
    static constexpr auto EDITOR_BUFFER_QUAD_OFFSET = INFO_BAR_BUFFER_QUAD_COUNT + PROMPT_BUFFER_QUAD_COUNT;

    /** Maximum quads available for the editor view */
    static constexpr auto EDITOR_BUFFER_QUAD_COUNT = 8192 - EDITOR_BUFFER_QUAD_OFFSET;

private:
    /** SDL window handle. */
    SDL_Window* p_sdl_window;

    /** OpenGL rendering context. */
    SDL_GLContext m_sdl_gl_context;

    /** Command system manager (commands, CVars, history, etc.). */
    CommandManager m_command_manager;

    /** Theme manager for fonts, colors, and UI style. */
    Theme m_theme;

    /** Shader program used to render textured quads. */
    QuadProgram m_quad_program;

    /** Geometry buffer for batched quad rendering. */
    QuadBuffer m_quad_buffer;

    /** The prompt cursor. */
    PromptCursor m_prompt_cursor;

    /** The only cursor for now */
    CursorContext m_cursor_context;

    /** Top info bar view. */
    InfoBar m_info_bar;

    /** Main editor view. */
    Editor m_editor;

    /** Bottom command prompt view. */
    Prompt m_prompt;

    /** State tracking the info bar. */
    ViewState m_info_bar_state;

    /** State object tracking the editor. */
    ViewState m_editor_state;

    /** State object tracking the prompt. */
    PromptState m_prompt_state;

    /** CVar tracking the maximum frame time (to render, before swapping). */
    std::shared_ptr<CVarFloat> m_command_time;

    /** CVar tracking the maximum frame time (to render, before swapping). */
    std::shared_ptr<CVarFloat> m_draw_time;

    /** The bind command. */
    std::shared_ptr<BindCommand> m_bind_command;

    /** 4x4 orthogonal projection matrix for 2D rendering. */
    std::array<float, 16> m_orthogonal;

    /**
     * @brief Recomputes the orthogonal projection matrix.
     *
     * @param width New window width.
     * @param height New window height.
     */
    void updateOrthogonal(int32_t width, int32_t height);

    /**
     * @brief Run the said command.
     *
     * @param command The command string to rexecute by m_command_manager.
     * @param fromPrompt If the command is running from a direct prompt input.
     */
    bool runCommand(std::u16string_view command, bool fromPrompt) override;

public:
    /** @brief Deleted copy constructor. */
    ApplicationWindow(const ApplicationWindow &) = delete;

    /** @brief Deleted copy assignment operator. */
    ApplicationWindow &operator=(const ApplicationWindow &) = delete;

    ~ApplicationWindow() override = default;

    /** @brief Constructs the ApplicationWindow with default values. */
    explicit ApplicationWindow();

    /**
     * @brief Creates the SDL window and initializes OpenGL context.
     *
     * @param title The window title.
     * @param width Initial window width in pixels.
     * @param height Initial window height in pixels.
     */
    void create(std::string_view title, int32_t width, int32_t height);

    /** @brief Cleans up all allocated resources and destroys the window. */
    void destroy();

    /** @brief Starts the main application loop (event handling and rendering). */
    void mainLoop();

    /**
     * @brief Provides command name completions for the command prompt.
     *
     * Invoked when the user is typing a command; filters available commands based on partial input. It use
     * CommandManager under the hood to retrieve the items. Parts of CommandRunner.
     *
     * @param input Partial command name typed by the user.
     * @param itemCallback Callback to return matching command names.
     */
    void getCommandCompletions(std::u16string_view input, const AutoCompleteCallback &itemCallback) override;


    /**
     * @brief Provides argument completions for a specific command.
     *
     * Use CommandManager under the hood to provide argument completions.
     * Parts of CommandRunner.
     *
     * @param command Name of the command whose arguments are being completed. ///< Target command.
     * @param argumentIndex Index of the current argument being typed.         ///< Zero-based arg index.
     * @param input Partial input for the current argument.                    ///< Filter string.
     * @param itemCallback Callback to return matching argument suggestions.   ///< Suggestion callback.
     */
    void getArgumentsCompletions(std::u16string_view command, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) override;
};


#endif //APPLICATION_WINDOW_H
