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
#include "ApplicationWindow.h"

#include <algorithm>
#include <filesystem>
#include <limits>
#include <system_error>

#include <memory>
#include <stdexcept>

#include <SDL_image.h>
#include <glad/glad.h>
#include <utf8.h>

// Supplied by the selected renderer source set (gl45 or gl43) via the CMake include path
#include <GlBackend.h>

#include "command/ActivatePromptCommand.h"
#include "command/AutoCompleteCommand.h"
#include "command/BindCommand.h"
#include "command/BufferCommand.h"
#include "command/CopyTextCommand.h"
#include "command/CutTextCommand.h"
#include "command/ExecCommand.h"
#include "command/FontSizeCommand.h"
#include "command/GotoLineCommand.h"
#include "command/HelpCommand.h"
#include "command/MoveCursorCommand.h"
#include "command/OpenFileCommand.h"
#include "command/OskCommand.h"
#include "command/PasteTextCommand.h"
#include "command/PromptCommand.h"
#include "command/QuitCommand.h"
#include "command/RedoCommand.h"
#include "command/ResetCVarFloatCommand.h"
#include "command/SaveFileCommand.h"
#include "command/SearchCommand.h"
#include "command/SetHighLightCommand.h"
#include "command/UndoCommand.h"
#include "core/base/CommandLine.h"
#include "core/theme/DimensionId.h"
#include "core/FocusTarget.h"
#include "platform/Platform.h"


namespace {
    /**
     * @brief Resolves which autoexec to run, seeding a user-editable copy where assets are read-only.
     *
     * Where the platform exposes a user config directory (Switch: next to bbloc.nro, the packaged
     * romfs being read-only there), the script living in it wins, and the packaged one is copied
     * over the first time so there is always a file the user can edit. Anything going wrong — no
     * directory, an unwritable destination — falls back to the packaged script.
     *
     * @param packagedPath The shipped autoexec, already resolved through Platform::assetPath. UTF-8.
     * @param userDir The directory returned by Platform::userConfigDir, if any. UTF-8.
     * @return The path to hand to the exec command. UTF-8.
     */
    std::string resolveAutoexecPath(const std::string &packagedPath, const std::optional<std::string> &userDir) {
        if (!userDir.has_value()) {
            return packagedPath;
        }

        auto error_code = std::error_code{};
        const auto user_path = *userDir + "autoexec";
        if (std::filesystem::is_regular_file(user_path, error_code)) {
            return user_path;
        }

        if (!std::filesystem::copy_file(packagedPath, user_path, error_code)) {
            // A half-written script would win over the packaged one on every later run.
            std::filesystem::remove(user_path, error_code);
            return packagedPath;
        }
        return user_path;
    }
}


ApplicationWindow::ApplicationWindow()
    : p_sdl_window(nullptr),
      m_sdl_gl_context(nullptr),
      m_max_undo(std::make_shared<CVarInt>(64)),
      m_context_manager(*this, m_theme, m_prompt_cursor, m_max_undo),
      m_info_bar(m_command_manager, m_theme, m_quad_program),
      m_editor(m_command_manager, m_theme, m_quad_program),
      m_prompt(m_command_manager, m_theme, m_quad_program),
      m_osk(m_command_manager, m_theme, m_quad_program),
      m_prompt_state(m_command_manager),
      m_command_time(std::make_shared<CVarFloat>(0.0f, true)),
      m_draw_time(std::make_shared<CVarFloat>(0.0f, true)),
      m_search_case_sensitive(std::make_shared<CVarBool>(false)),
      m_bind_command(std::make_shared<BindCommand>(m_command_manager)),
      m_orthogonal(),
      m_keyboard_input(*this, m_context_manager, m_editor, m_editor_state, m_prompt, m_prompt_state),
      m_pointer_input(m_context_manager, m_theme, m_info_bar, m_info_bar_state, m_editor, m_editor_state, m_prompt, m_prompt_state, m_osk, m_osk_state),
      m_controller_input(*this, m_context_manager, m_osk, m_osk_state) {}

bool ApplicationWindow::runBoundCommand(const SDL_Keycode keycode, const uint16_t modifiers) {
    if (const auto command = m_bind_command->getBinding(keycode, modifiers)) {
        const auto current_time = SDL_GetPerformanceCounter();
        if (runCommand(command.value(), false)) {
            const auto performance_query = static_cast<float>(SDL_GetPerformanceFrequency());
            const auto command_time_elapsed = static_cast<float>(SDL_GetPerformanceCounter() - current_time) / performance_query;
            if (command_time_elapsed > m_command_time->m_value) {
                m_command_time->m_value = command_time_elapsed;
            }

            return true;
        }
    }

    return false;
}

void ApplicationWindow::updateOrthogonal(const int32_t width, const int32_t height) {
    const auto right = static_cast<float>(width);
    const auto bottom = static_cast<float>(height);
    constexpr auto left = 0.0f;
    constexpr auto top = 0.0f;
    constexpr auto near = 0.0f;
    constexpr auto far = 1.0f;

    // Calculate the new orthogonal matrice
    m_orthogonal[0] = 2.0f / (right - left);
    m_orthogonal[3] = -(right + left) / (right - left);
    m_orthogonal[5] = 2.0f / (top - bottom);
    m_orthogonal[7] = -(top + bottom) / (top - bottom);
    m_orthogonal[10] = -2.0f / (far - near);
    m_orthogonal[11] = -(far + near) / (far - near);
    m_orthogonal[15] = 1.0f;
}

void ApplicationWindow::create(const std::string &title, const int32_t width, const int32_t height, const int32_t argc, const char *argv[]) {
    // Touch is handled explicitly in mainLoop; stop SDL from synthesizing mouse events from fingers
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");

    // Init SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        throw std::runtime_error(std::string("Failed to initialize SDL: ").append(SDL_GetError()));
    }

    // Platform mapping overrides must be in place before the first CONTROLLERDEVICEADDED
    // is polled, which opens the controller with whatever mapping is registered.
    Platform::addControllerMappings();

    // Set the OpenGL Core context version required by the compiled renderer backend
    // (4.5 for the DSA set, 4.3 for the bind-based set) and double buffered RGB8 surface
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, GlBackend::CONTEXT_MAJOR_VERSION);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, GlBackend::CONTEXT_MINOR_VERSION);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);

    constexpr auto window_position = SDL_WINDOWPOS_CENTERED;
    constexpr auto window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE;
    p_sdl_window = SDL_CreateWindow(title.c_str(), window_position, window_position, width, height, window_flags);

    if (p_sdl_window == nullptr) {
        throw std::runtime_error(std::string("Failed to create SDL window: ").append(SDL_GetError()));
    }

    // Create OpenGL context
    m_sdl_gl_context = SDL_GL_CreateContext(p_sdl_window);
    if (m_sdl_gl_context == nullptr) {
        throw std::runtime_error("Failed to create OpenGL context");
    }
    
    SDL_GL_MakeCurrent(p_sdl_window, m_sdl_gl_context);
    SDL_GL_SetSwapInterval(1);
    gladLoadGL();
    // Set our default OpenGL states
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glCullFace(GL_BACK);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Create the theme
    const auto path = Platform::assetPath("romfs/");
    m_theme.create(m_command_manager, path);

    // Create the quad buffer
    updateOrthogonal(width, height);
    m_quad_buffer.create(DEFAULT_QUAD_CAPACITY);

    // Create the quad shader
    m_quad_program.create();
    m_quad_program.use();
    m_quad_program.bindVertexBuffer(m_quad_buffer.getBuffer());
    m_quad_program.setMatrix(m_orthogonal.data());

    // Create the views
    m_info_bar.resizeWindow(width, height);
    m_editor.resizeWindow(width, height);
    m_prompt.resizeWindow(width, height);
    m_osk.resizeWindow(width, height);

    // Register cvars and commands then run autoexec
    m_command_manager.registerCvar(u"inf_draw_time", m_draw_time, nullptr);
    m_command_manager.registerCvar(u"inf_command_time", m_command_time, nullptr);
    m_command_manager.registerCvar(u"dim_max_undo", m_max_undo, [this] {
        // Clamp the depth so the user cannot exhaust memory or disable history entirely.
        m_max_undo->m_value = std::clamp(m_max_undo->m_value, 1, 4096);
        // Every open context shares the depth CVar: trim them all.
        m_context_manager.applyMaxHistoryDepth();
    });
    m_command_manager.registerCvar(u"search_case_sensitive", m_search_case_sensitive, [this] {
        // Re-count matches for the stored term in place so the indicator reflects the new mode immediately.
        SearchCommand::refreshMatchStats(m_context_manager.active(), m_search_case_sensitive->m_value);
    });
    m_command_manager.registerCommand(u"quit", std::make_shared<QuitCommand>(m_context_manager), false, false);
    m_command_manager.registerCommand(u"open", std::make_shared<OpenFileCommand>(m_context_manager), false, false);
    m_command_manager.registerCommand(u"buffer", std::make_shared<BufferCommand>(m_context_manager), false, false);
    m_command_manager.registerCommand(u"save", std::make_shared<SaveFileCommand>(), false, false);
    m_command_manager.registerCommand(u"reset_draw_time", std::make_shared<ResetCVarFloatCommand>(m_draw_time), false, false);
    m_command_manager.registerCommand(u"reset_command_time", std::make_shared<ResetCVarFloatCommand>(m_command_time), false, false);
    m_command_manager.registerCommand(u"set_font_size", std::make_shared<FontSizeCommand>(), false, false);
    m_command_manager.registerCommand(u"set_hl_mode", std::make_shared<SetHighLightCommand>(), false, false);
    m_command_manager.registerCommand(u"bind", m_bind_command, false, false);
    m_command_manager.registerCommand(u"activate_prompt", std::make_shared<ActivatePromptCommand>(m_prompt_state), true, false);
    m_command_manager.registerCommand(u"prompt", std::make_shared<PromptCommand>(m_prompt, m_prompt_state), true, true);
    m_command_manager.registerCommand(u"copy", std::make_shared<CopyTextCommand>(), false, false);
    m_command_manager.registerCommand(u"paste", std::make_shared<PasteTextCommand>(), false, false);
    m_command_manager.registerCommand(u"cut", std::make_shared<CutTextCommand>(), false, false);
    m_command_manager.registerCommand(u"undo", std::make_shared<UndoCommand>(), false, false);
    m_command_manager.registerCommand(u"redo", std::make_shared<RedoCommand>(), false, false);
    m_command_manager.registerCommand(u"move", std::make_shared<MoveCursorCommand>(m_prompt_state), false, true);
    m_command_manager.registerCommand(u"goto_line", std::make_shared<GotoLineCommand>(), false, false);
    m_command_manager.registerCommand(u"search", std::make_shared<SearchCommand>(SearchCommand::Action::Search, m_search_case_sensitive), false, false);
    m_command_manager.registerCommand(u"find_next", std::make_shared<SearchCommand>(SearchCommand::Action::FindNext, m_search_case_sensitive), false, false);
    m_command_manager.registerCommand(u"find_prev", std::make_shared<SearchCommand>(SearchCommand::Action::FindPrev, m_search_case_sensitive), false, false);
    m_command_manager.registerCommand(u"replace", std::make_shared<SearchCommand>(SearchCommand::Action::Replace, m_search_case_sensitive), false, false);
    m_command_manager.registerCommand(u"replace_all", std::make_shared<SearchCommand>(SearchCommand::Action::ReplaceAll, m_search_case_sensitive), false, false);
    m_command_manager.registerCommand(u"exec", std::make_shared<ExecCommand>(), false, false);
    m_command_manager.registerCommand(u"auto_complete", std::make_shared<AutoCompleteCommand>(m_prompt_state), true, true);
    m_command_manager.registerCommand(u"osk", std::make_shared<OskCommand>(m_osk_state), false, true);
    m_command_manager.registerCommand(u"help", std::make_shared<HelpCommand>(m_context_manager), false, false);

    // Follow the system color scheme where the platform exposes one (Switch console color set);
    // runs before autoexec so the colors set there win, the system scheme being just the default
    if (const auto color_scheme = Platform::preferredColorScheme(); color_scheme.has_value()) {
        const auto theme_script = *color_scheme == Platform::ColorScheme::Dark ? u"dark_theme" : u"light_theme";
        runCommand(std::u16string(u"exec ").append(utf8::utf8to16(path)).append(theme_script), false);
    }

    // Don't run it "from prompt", so its not added to history. The path is quoted: the user copy
    // sits wherever the executable does, which may be a directory with spaces in its name.
    const auto autoexec_path = resolveAutoexecPath(path + "autoexec", Platform::userConfigDir(argc > 0 ? argv[0] : ""));
    runCommand(std::u16string(u"exec \"").append(utf8::utf8to16(autoexec_path)).append(u"\""), false);

    if (argc > 1) {
        // Only the first path is opened; it loads into the pristine startup buffer
        openFile(argv[1]);
    }
}

void ApplicationWindow::openFile(const std::string_view path) {
    auto utf16_path = std::u16string{};
    try {
        utf16_path = utf8::utf8to16(path);
    } catch (const utf8::exception &) {
        m_prompt_state.setRunningState(PromptState::RunningState::Message);
        resetPrompt(u"Invalid UTF-8 encoding in path.");
        return;
    }

    // Quote the path so tokenize keeps it as a single argument even with spaces
    runCommand(std::u16string(u"open \"").append(utf16_path).append(u"\""), false);
}

void ApplicationWindow::mainLoop() {
    // Request performance query used to calculate dt time
    const auto performance_query = static_cast<float>(SDL_GetPerformanceFrequency());

    auto window_width = 0;
    auto window_height = 0;
    auto is_running = true;
    auto last_time = SDL_GetPerformanceCounter();
    SDL_GetWindowSize(p_sdl_window, &window_width, &window_height);
    SDL_ShowWindow(p_sdl_window);

    SDL_Event event;
    while (is_running) {
        // Wait events from SDL; with a repeat armed (controller input, held OSK key), wake at
        // the earliest deadline instead of blocking indefinitely (clamped to at least 1 ms)
        auto repeat_deadline = std::numeric_limits<uint64_t>::max();
        if (m_controller_input.isRepeatArmed()) {
            repeat_deadline = m_controller_input.getRepeatDeadline();
        }

        if (m_osk_state.getRepeater().isArmed()) {
            repeat_deadline = std::min(repeat_deadline, m_osk_state.getRepeater().getDeadline());
        }

        if (repeat_deadline != std::numeric_limits<uint64_t>::max()) {
            const auto remaining = static_cast<int64_t>(repeat_deadline) - static_cast<int64_t>(SDL_GetTicks64());
            SDL_WaitEventTimeout(nullptr, static_cast<int32_t>(std::max<int64_t>(remaining, 1)));
        } else {
            SDL_WaitEvent(nullptr);
        }

        while (SDL_PollEvent(&event)) {
            // Synthesized OSK text arrives as a user event (see Osk::textEventType): deliver
            // it exactly like an SDL_TEXTINPUT, so everything downstream stays unaware.
            if (event.type == Osk::textEventType()) {
                auto text_event = SDL_TextInputEvent{};
                text_event.type = SDL_TEXTINPUT;
                text_event.timestamp = event.user.timestamp;
                SDL_strlcpy(text_event.text, static_cast<char *>(event.user.data1), sizeof(text_event.text));
                SDL_free(event.user.data1);
                dismissMessage();
                m_keyboard_input.onTextInput(text_event);
                continue;
            }

            switch (event.type) {
                case SDL_QUIT:
                    is_running = false;
                break;
                case SDL_CONTROLLERDEVICEADDED:
                    m_controller_input.onDeviceAdded(event.cdevice);
                break;
                case SDL_CONTROLLERDEVICEREMOVED:
                    m_controller_input.onDeviceRemoved(event.cdevice);
                break;
                case SDL_CONTROLLERBUTTONDOWN:
                    // No dismissal here: ControllerInput::press does it, so the axis-derived
                    // presses (sticks, triggers) dismiss too.
                    m_controller_input.onButtonDown(event.cbutton);
                break;
                case SDL_CONTROLLERBUTTONUP:
                    m_controller_input.onButtonUp(event.cbutton);
                break;
                case SDL_CONTROLLERAXISMOTION:
                    m_controller_input.onAxisMotion(event.caxis);
                break;
                case SDL_WINDOWEVENT:
                    switch (event.window.event) {
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                            window_width = event.window.data1;
                            window_height = event.window.data2;
                            updateOrthogonal(window_width, window_height);
                            m_quad_program.setMatrix(m_orthogonal.data());
                            m_info_bar.resizeWindow(window_width, window_height);
                            m_editor.resizeWindow(window_width, window_height);
                            m_prompt.resizeWindow(window_width, window_height);
                            m_osk.resizeWindow(window_width, window_height);
                            m_context_manager.active().wants_redraw = true;
                        break;
                        default:
                        break;
                    }
                break;
                case SDL_KEYDOWN:
                    dismissMessage();
                    m_keyboard_input.onKeyDown(event.key);
                break;
                case SDL_TEXTINPUT:
                    dismissMessage();
                    m_keyboard_input.onTextInput(event.text);
                break;
                case SDL_MOUSEBUTTONDOWN:
                    dismissMessage();
                    m_pointer_input.onMouseDown(event.button);
                break;
                case SDL_MOUSEMOTION:
                    m_pointer_input.onMouseMotion(event.motion);
                break;
                case SDL_MOUSEBUTTONUP:
                    m_pointer_input.onMouseUp(event.button);
                break;
                case SDL_MOUSEWHEEL:
                    m_pointer_input.onMouseWheel(event.wheel);
                break;
                case SDL_FINGERDOWN:
                    dismissMessage();
                    m_pointer_input.onFingerDown(event.tfinger, window_width, window_height);
                break;
                case SDL_FINGERMOTION:
                    m_pointer_input.onFingerMotion(event.tfinger, window_width, window_height);
                break;
                case SDL_FINGERUP:
                    m_pointer_input.onFingerUp(event.tfinger, window_width, window_height);
                break;
                default:
                break;
            }
        }

        // Fire the armed repeats after the poll loop, so fresh events (a release, a new
        // press) disarm or replace them first
        m_controller_input.tickRepeat();
        m_osk.tickRepeat(m_context_manager.active(), m_osk_state);

        // Calculate dt time
        const auto current_time = SDL_GetPerformanceCounter();
        const auto dt = static_cast<float>(current_time - last_time) / performance_query;
        last_time = current_time;
        // The views always render the active context; fetch it after the events, which may have switched it.
        auto &context = m_context_manager.active();
        if (context.wants_redraw) {
            // Need to redraw the whole views. A visible on-screen keyboard takes a bottom
            // strip; the prompt sits above it and the editor shrinks — the same path a
            // window resize takes, so nothing else needs to know about the OSK.
            const auto border_size = m_theme.getDimension(DimensionId::BorderSize);
            const auto line_height = m_theme.getLineHeight();
            const auto bar_height = line_height + border_size;
            const auto bar_width = window_width;
            const auto osk_height = m_osk_state.isVisible() ? window_height * m_theme.getDimension(DimensionId::OskHeight) / 100 : 0;

            m_info_bar_state.setPosition(0, 0);
            m_info_bar_state.setSize(bar_width, bar_height);

            m_prompt_state.setPosition(0, window_height - osk_height - bar_height);
            m_prompt_state.setSize(bar_width, bar_height);

            m_editor_state.setPosition(0, bar_height);
            m_editor_state.setSize(bar_width, std::max(0, window_height - bar_height * 2 - osk_height));

            m_osk_state.setPosition(0, window_height - osk_height);
            m_osk_state.setSize(bar_width, osk_height);

            glViewport(0, 0, window_width, window_height);
            glScissor(0, 0, window_width, window_height);
            glClearColor(0.0f, 0.0, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // Render everything on screen.
            context.highlighter.parse();
            m_quad_buffer.resetFrame();
            m_info_bar.render(context, m_info_bar_state, m_quad_buffer, dt);
            m_editor.render(context, m_editor_state, m_quad_buffer, dt);
            m_prompt.render(context, m_prompt_state, m_quad_buffer, dt);
            m_osk.render(context, m_osk_state, m_quad_buffer, dt);

            // todo: Uncomment for debug purpose.
            // std::cout << "view updated " << std::endl;
            context.wants_redraw = false;

            // Update max_render_time metrics before the swap, which blocks on vsync
            const auto frame_time_elapsed = static_cast<float>(SDL_GetPerformanceCounter() - current_time) / performance_query;
            if (frame_time_elapsed > m_draw_time->m_value) {
                m_draw_time->m_value = frame_time_elapsed;
            }

            SDL_GL_SwapWindow(p_sdl_window);
        }

        // Reset follow_indicator if it was not held by the editor render already
        context.scroll.follow_indicator = false;
    }
}

void ApplicationWindow::getCommandCompletions(const std::u16string_view input, const AutoCompleteCallback &itemCallback) {
    m_command_manager.getCommandCompletions(input, false, itemCallback);
}

void ApplicationWindow::getArgumentsCompletions(const std::u16string_view command, const std::span<const std::u16string_view> previousArgs, const int32_t argumentIndex, const std::u16string_view input, const AutoCompleteCallback &itemCallback) {
    m_command_manager.getArgumentsCompletion(command, previousArgs, argumentIndex, input, itemCallback);
}

void ApplicationWindow::destroy() {
    // Destroy renderer objects
    m_quad_program.destroy();
    m_quad_buffer.destroy();
    m_theme.destroy();

    // Exit SDL
    SDL_GL_DeleteContext(m_sdl_gl_context);
    SDL_DestroyWindow(p_sdl_window);
    SDL_Quit();

    // Default states
    m_sdl_gl_context = nullptr;
    p_sdl_window = nullptr;
    m_orthogonal = {};
}

void ApplicationWindow::dismissMessage() {
    if (m_prompt_state.getRunningState() == PromptState::RunningState::Message) {
        m_prompt_state.setRunningState(PromptState::RunningState::Idle);
        m_prompt_state.setPromptText(PromptState::PROMPT_READY);
        // The dismissing event may consume no view (an unbound key): redraw so the
        // screen cannot keep showing a message the state no longer holds.
        m_context_manager.active().wants_redraw = true;
    }
}

void ApplicationWindow::resetPrompt(const std::u16string_view promptText) {
    m_prompt_state.setPromptText(promptText);
    m_prompt_state.clearCompletions();
    m_prompt_state.clearHistoryIndex();
    m_prompt_cursor.clear();
    m_context_manager.active().wants_redraw = true;
}

bool ApplicationWindow::runCommand(const std::u16string_view command, const bool fromPrompt) {
    std::optional<std::u16string> result;
    // The context active when the command starts; the command itself may switch the active one.
    auto &context = m_context_manager.active();
    // Remember which feedback was pending, not merely that one was: a command replacing a pending
    // feedback with its own must still refresh the prompt, and identity is the only way to see it.
    const auto pending_feedback_id = context.command_feedback ? context.command_feedback->id : 0;
    const auto feedback_was_pending = pending_feedback_id != 0;
    if (fromPrompt && feedback_was_pending) {
        // The prompt input answers the pending feedback instead of being a command.
        // Copy the feedback object so the string is still valid after reset is called.
        const auto feedback = context.command_feedback.value();
        const auto feedback_answer = m_prompt_cursor.getString();
        context.command_feedback.reset();

        // Forward the raw answer so terms containing spaces survive (e.g. search feedback).
        // An empty answer flows to the command too, so it can report its usage.
        result = feedback.on_validate_callback(feedback_answer, feedback.command_string);
    } else {
        // Take the scratch by move so a nested runCommand (e.g. exec running script lines) sees an
        // empty member and allocates its own vector, keeping this command's args span valid.
        auto tokens = std::move(m_token_scratch);
        CommandLine::tokenize(command, tokens);
        if (tokens.empty()) {
            m_token_scratch = std::move(tokens);
            return false;
        }

        // An active prompt (the user typing a command, or a pending feedback question) is modal for
        // key bindings: running a bound command here could evict the question through the message
        // branch below, edit the buffer behind it, or steal the focus. Drop the command before it
        // runs, without touching the prompt, the feedback or the focus. Prompt input itself
        // (fromPrompt) and the commands the prompt machinery relies on stay executable; the mouse
        // side already preserves the interaction by never moving the keyboard focus.
        const auto prompt_is_active = context.focus_target == FocusTarget::Prompt || feedback_was_pending;
        if (!fromPrompt && prompt_is_active && !m_command_manager.isAllowedDuringPrompt(tokens[0])) {
            m_token_scratch = std::move(tokens);
            return false;
        }

        if (fromPrompt) {
            m_prompt_state.addHistory(command);

            // Move focus to the editor if we run this command from the prompt,
            // because we don't want the next command to apply in the prompt in this case (e.g: "move up").
            context.focus_target = FocusTarget::Editor;
        }

        context.from_prompt = fromPrompt;
        result = m_command_manager.run(context, tokens);

        // The command is done with its args span: give the capacity back for the next call.
        m_token_scratch = std::move(tokens);
    }

    // The command may have switched the active context (e.g. "buffer next"):
    // the prompt and focus updates below must apply to the new active one.
    auto &active_context = m_context_manager.active();
    if (result) {
        // Show the error message in the prompt, if any. The message replaces a pending
        // feedback, so drop it to not consume the next prompt input as its answer.
        active_context.command_feedback.reset();
        m_prompt_state.setRunningState(PromptState::RunningState::Message);
        resetPrompt(*result);

        // Focus go to the editor
        active_context.focus_target = FocusTarget::Editor;
    } else if (active_context.command_feedback) {
        // A feedback pending before this call survives a bound command untouched:
        // the prompt already shows it. Only a newly requested feedback updates the prompt.
        if (active_context.command_feedback->id != pending_feedback_id) {
            m_prompt_state.setRunningState(PromptState::RunningState::Running);
            resetPrompt(active_context.command_feedback->prompt_message);

            // Focus go to the prompt
            active_context.focus_target = FocusTarget::Prompt;
            active_context.search.resetMatches();
        }
    } else {
        // The prompt state can change while command execution (e.g: activate_prompt, cancel), check it again.
        switch (m_prompt_state.getRunningState()) {
            case PromptState::RunningState::Idle:
                resetPrompt(PromptState::PROMPT_READY);
                active_context.focus_target = FocusTarget::Editor;
            break;
            default:
                // Don't change anything
            break;
        }
    }

    return true;
}
