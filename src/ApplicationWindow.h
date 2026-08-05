/*
* Copyright (C) 2026 Romain Graillot
 *
 * This file is part of bbloc.
 *
 * bbloc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bbloc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef APPLICATION_WINDOW_H
#define APPLICATION_WINDOW_H

#include <array>
#include <span>
#include <string_view>
#include <vector>

#include <SDL.h>

#include "core/cvar/CVarBool.h"
#include "core/cvar/CVarFloat.h"
#include "core/cvar/CVarInt.h"
#include "core/CommandManager.h"
#include "core/cursor/PromptCursor.h"
#include "core/renderer/QuadBuffer.h"
#include "core/renderer/QuadProgram.h"
#include "core/theme/Theme.h"
#include "core/CursorContextManager.h"
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
    /** Initial capacity of m_quad_buffer in quads; the buffer regrows on demand. */
    static constexpr uint32_t DEFAULT_QUAD_CAPACITY = 8192;

    /** Default quad count reserved for the info bar view batch */
    static constexpr uint32_t INFO_BAR_DEFAULT_QUAD_COUNT = 1024;

    /** Default quad count reserved for the prompt view batch */
    static constexpr uint32_t PROMPT_DEFAULT_QUAD_COUNT = 1024;

    /** Default quad count reserved for the editor view batch */
    static constexpr uint32_t EDITOR_DEFAULT_QUAD_COUNT = DEFAULT_QUAD_CAPACITY - INFO_BAR_DEFAULT_QUAD_COUNT - PROMPT_DEFAULT_QUAD_COUNT;

private:
    /**
     * @brief Views a mouse press can be routed to.
     *
     * The view under a left-button press captures the pointer: motion and release events keep
     * being routed to it until the button is released, even when the pointer leaves the view.
     */
    enum class MouseTarget : uint8_t {
        None,     ///< No press in progress.
        InfoBar,  ///< The info bar received the press.
        Editor,   ///< The editor received the press.
        Prompt    ///< The prompt received the press.
    };

    /** SDL window handle. */
    SDL_Window *p_sdl_window;

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

    /** CVar tracking the maximum undo/redo history depth; declared before the context manager, which shares it with every cursor. */
    std::shared_ptr<CVarInt> m_max_undo;

    /** Open cursor contexts, one per file; the views always render the active one. */
    CursorContextManager m_context_manager;

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

    /** CVar tracking the maximum command execution time. */
    std::shared_ptr<CVarFloat> m_command_time;

    /** CVar tracking the maximum frame time (to render, before swapping). */
    std::shared_ptr<CVarFloat> m_draw_time;

    /** CVar tracking whether searches match case. */
    std::shared_ptr<CVarBool> m_search_case_sensitive;

    /** The bind command. */
    std::shared_ptr<BindCommand> m_bind_command;

    /** 4x4 orthogonal projection matrix for 2D rendering. */
    std::array<float, 16> m_orthogonal;

    /** View that received the current left-button press, None outside a press. */
    MouseTarget m_mouse_target;

    /** Scratch vector whose capacity is reused by runCommand to tokenize command strings. */
    std::vector<std::u16string_view> m_token_scratch;

    /**
     * @brief Recomputes the orthogonal projection matrix.
     *
     * @param width New window width.
     * @param height New window height.
     */
    void updateOrthogonal(int32_t width, int32_t height);

    /**
     * @brief Tells whether a window point lies inside a view rectangle.
     *
     * @param viewState The view state holding the rectangle to test.
     * @param x Window-relative x coordinate of the point, in pixels.
     * @param y Window-relative y coordinate of the point, in pixels.
     * @return true when the point is inside the rectangle, false otherwise.
     */
    [[nodiscard]] static bool viewContains(const ViewState &viewState, int32_t x, int32_t y);

    /**
     * @brief Resets the prompt line to display the given text.
     *
     * Clears the completions, the history navigation and the prompt cursor, then requests a redraw.
     *
     * @param promptText New prompt label text.
     */
    void resetPrompt(std::u16string_view promptText);

    /**
     * @brief Run the said command.
     *
     * While the prompt is active (focused, or holding a pending feedback question), key-bound
     * commands not registered as allowed during the prompt are silently dropped, so a shortcut
     * cannot evict the interaction. Prompt input (fromPrompt) is never dropped.
     *
     * @param command The command string to rexecute by m_command_manager.
     * @param fromPrompt If the command is running from a direct prompt input.
     * @return true when a command ran, false when the input was empty or dropped by the prompt gate.
     */
    bool runCommand(std::u16string_view command, bool fromPrompt) override;

    /**
     * @brief Opens the file at the given path in the editor.
     *
     * Delegates to the "open" command; must be called after commands are registered.
     * Errors (missing file, invalid encoding) are shown as a prompt message.
     *
     * @param path UTF-8 encoded path of the file to open.
     */
    void openFile(std::string_view path);

public:
    /** @brief Deleted copy constructor. */
    ApplicationWindow(const ApplicationWindow &) = delete;

    /** @brief Deleted copy assignment operator. */
    ApplicationWindow &operator=(const ApplicationWindow &) = delete;

    /** @brief Release resources helds by ApplicationWindow. */
    ~ApplicationWindow() override = default;

    /** @brief Constructs the ApplicationWindow with default values. */
    explicit ApplicationWindow();

    /**
     * @brief Creates the SDL window and initializes OpenGL context.
     *
     * After command registration and the autoexec run, opens the file given as first
     * program argument, if any.
     *
     * @param title The window title.
     * @param width Initial window width in pixels.
     * @param height Initial window height in pixels.
     * @param argc Program argument count, as received by main().
     * @param argv Program argument values, as received by main().
     */
    void create(std::string_view title, int32_t width, int32_t height, int32_t argc, const char *argv[]);

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
     * @param command Name of the command whose arguments are being completed.
     * @param previousArgs The arguments typed before the one being completed, excluding the command name.
     * @param argumentIndex Index of the current argument being typed.
     * @param input Partial input for the current argument.
     * @param itemCallback Callback to return matching argument suggestions.
     */
    void getArgumentsCompletions(std::u16string_view command, std::span<const std::u16string_view> previousArgs, int32_t argumentIndex, std::u16string_view input, const AutoCompleteCallback &itemCallback) override;
};


#endif //APPLICATION_WINDOW_H
