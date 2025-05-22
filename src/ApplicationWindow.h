#ifndef APPLICATION_WINDOW_H
#define APPLICATION_WINDOW_H

#include <array>
#include <string_view>

#include <SDL.h>

#include "core/command/cvar/CVarFloat.h"
#include "core/CommandFeedback.h"
#include "core/command/CommandManager.h"
#include "core/cursor/PromptCursor.h"
#include "core/renderer/QuadBuffer.h"
#include "core/renderer/QuadProgram.h"
#include "core/theme/Theme.h"
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
class ApplicationWindow final {
private:
    /** @brief Represents the currently focused input target. */
    enum class FocusTarget {
        Editor, ///< Editor view is focused.
        Prompt  ///< Prompt view is focused.
    };

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

    /** CVar tracking the maximum frame time (to render, before swapping). */
    std::shared_ptr<CVarFloat> m_render_time;

    /** Map of key binding to commands */
    std::unordered_map<SDL_Keycode, std::unordered_map<uint16_t, std::u16string>> m_key_bindings;

    /** The prompt cursor. */
    PromptCursor m_prompt_cursor;

    /** The only cursor for now */
    CursorContext m_cursor_context;

    /** Top info bar view. */
    InfoBar m_info_bar;

    /** Main editor view. */
    Editor m_editor;

    /** State tracking the info bar. */
    ViewState m_info_bar_state;

    /** State object tracking the editor. */
    ViewState m_editor_state;

    /** State object tracking the prompt. */
    PromptState m_prompt_state;

    /** Bottom command prompt view. */
    Prompt m_prompt;

    /** Active feedback prompt state. */
    std::optional<CommandFeedback> m_command_feedback;

    /** The current focused view */
    FocusTarget m_focus_target;

    /** 4x4 orthogonal projection matrix for 2D rendering. */
    std::array<float, 16> m_orthogonal;

    /**
      * @brief Provides completions for interactive feedback input.
      * @param itemCallback Callback receiving feedback suggestions.
      */
    void getFeedbackCompletion(const ItemCallback<char16_t> &itemCallback) const;

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

    /** @brief Registers the bind command. */
    void registerBindCommand();

    /** @brief Registers the increase_font_size and decrease_font_size command. */
    void registerFontSizeCommand();

    /** @brief Registers the activate_prompt command. */
    void registerActivatePromptCommand();

    /** @brief Registers the copy command. */
    void registerCopyCommand();

    /** @brief Registers the paste command. */
    void registerPasteCommand();

    /** @brief Registers the cut command. */
    void registerCutCommand();

    /** @brief Registers a command to change highlight mode. */
    void registerHighlighterCommand();

    /** @brief Registers the exec command. */
    void registerExecCommand();

    /** @brief Registers the commands to move the cursors. */
    void registerMoveCommands();

    /** @brief Registers the cancel command */
    void registerCancelCommand();

    /** @brief Registers the validate command */
    void registerValidateCommand();

    /** @brief Registers the auto_complete command */
    void registerAutoCompleteCommand();

    /**
     * @brief Run the said command.
     * @param command The command string to rexecute by m_command_manager.
     * @param fromPrompt If the command is running from a direct prompt input.
     */
    void runCommand(std::u16string_view command, bool fromPrompt);

    /** @brief Normalize input modifiers from raw sdl input modifiers*/
    static uint16_t normalizeModifiers(uint16_t modifiers);

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
