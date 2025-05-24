#include "ApplicationWindow.h"

#include <filesystem>

#include <memory>
#include <stdexcept>

#include <SDL_image.h>
#include <glad/glad.h>
#include <utf8.h>

#include "command/ActivatePromptCommand.h"
#include "command/AutoCompleteCommand.h"
#include "command/BindCommand.h"
#include "command/CancelCommand.h"
#include "command/CopyTextCommand.h"
#include "command/CutTextCommand.h"
#include "command/ExecCommand.h"
#include "command/FontSizeCommand.h"
#include "command/MoveCursorCommand.h"
#include "command/OpenFileCommand.h"
#include "command/PasteTextCommand.h"
#include "command/QuitCommand.h"
#include "command/ResetRenderTimeCommand.h"
#include "command/SaveFileCommand.h"
#include "command/SetHighLightCommand.h"
#include "command/ValidateCommand.h"
#include "core/cursor/buffer/StringBuffer.h"
#include "core/cursor/buffer/VectorBuffer.h"
#include "core/theme/DimensionId.h"
#include "core/FocusTarget.h"


ApplicationWindow::ApplicationWindow()
    : p_sdl_window(nullptr),
      m_sdl_gl_context(nullptr),
      m_cursor_context(*this, m_theme, m_prompt_cursor, std::make_unique<VectorBuffer>()),
      m_info_bar(m_command_manager, m_theme, m_quad_program, m_quad_buffer),
      m_editor(m_command_manager, m_theme, m_quad_program, m_quad_buffer),
      m_prompt(m_command_manager, m_theme, m_quad_program, m_quad_buffer),
      m_prompt_state(m_command_manager),
      m_render_time(std::make_shared<CVarFloat>(0.0f, true)),
      m_bind_command(std::make_shared<BindCommand>()),
      m_orthogonal() {}

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

void ApplicationWindow::create(const std::string_view title, const int32_t width, const int32_t height) {
    // Init SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw std::runtime_error(std::string("Failed to initialize SDL: ").append(SDL_GetError()));
    }

    // Set OpenGL 4.3 Core context and double buffered RGB8 surface
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);

    constexpr auto window_position = SDL_WINDOWPOS_CENTERED;
    constexpr auto window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE;
    p_sdl_window = SDL_CreateWindow(title.data(), window_position, window_position, width, height, window_flags);

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
    glDisable(GL_DEPTH_WRITEMASK);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glCullFace(GL_BACK);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Create the theme
    const auto path = std::string("romfs/");
    m_theme.create(m_command_manager, path);

    // Create the quad buffer
    updateOrthogonal(width, height);
    m_quad_buffer.create(MAX_QUADS);

    // Create the quad shader
    m_quad_program.create();
    m_quad_program.use();
    m_quad_program.bindVertexBuffer(m_quad_buffer.getBuffer());
    m_quad_program.setMatrix(m_orthogonal.data());

    // Create the views
    m_info_bar.resizeWindow(width, height);
    m_editor.resizeWindow(width, height);
    m_prompt.resizeWindow(width, height);

    // Register cvars and commands then run autoexec
    m_command_manager.registerCvar("inf_render_time", m_render_time, nullptr);
    m_command_manager.registerCommand("quit", std::make_shared<QuitCommand>());
    m_command_manager.registerCommand("open", std::make_shared<OpenFileCommand>());
    m_command_manager.registerCommand("save", std::make_shared<SaveFileCommand>());
    m_command_manager.registerCommand("reset_render_time", std::make_shared<ResetRenderTimeCommand>(m_render_time));
    m_command_manager.registerCommand("set_font_size", std::make_shared<FontSizeCommand>());
    m_command_manager.registerCommand("set_hl_mode", std::make_shared<SetHighLightCommand>());
    m_command_manager.registerCommand("bind", m_bind_command);
    m_command_manager.registerCommand("activate_prompt", std::make_shared<ActivatePromptCommand>(m_prompt_state));
    m_command_manager.registerCommand("copy", std::make_shared<CopyTextCommand>());
    m_command_manager.registerCommand("paste", std::make_shared<PasteTextCommand>());
    m_command_manager.registerCommand("cut", std::make_shared<CutTextCommand>());
    m_command_manager.registerCommand("move", std::make_shared<MoveCursorCommand>(m_prompt_state));
    m_command_manager.registerCommand("exec", std::make_shared<ExecCommand>());
    m_command_manager.registerCommand("cancel", std::make_shared<CancelCommand>(m_prompt_state));
    m_command_manager.registerCommand("validate", std::make_shared<ValidateCommand>(m_prompt_state));
    m_command_manager.registerCommand("auto_complete", std::make_shared<AutoCompleteCommand>(m_prompt_state));
    m_command_manager.run(m_cursor_context, { u"exec", utf8::utf8to16(path).append(u"autoexec") });
}

void ApplicationWindow::mainLoop() {
    // Request performance query used to calculate dt time
    const auto performanceQuery = static_cast<float>(SDL_GetPerformanceFrequency());
    auto window_width = 0;
    auto window_height = 0;
    auto is_running = true;
    auto lastTime = SDL_GetPerformanceCounter();
    SDL_GetWindowSize(p_sdl_window, &window_width, &window_height);
    SDL_ShowWindow(p_sdl_window);

    SDL_Event event;
    while (is_running) {
        // Wait events from SDL
        SDL_WaitEvent(nullptr);

        // We will use this to calculate the time spent to render this frame
        const auto frame_time = SDL_GetPerformanceCounter();
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    is_running = false;
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
                            m_cursor_context.wants_redraw = true;
                        break;
                        default:
                        break;
                    }
                break;
                case SDL_KEYDOWN: {
                    // Try to run command binding first
                    if (const auto command = m_bind_command->getBinding(event.key.keysym.sym, event.key.keysym.mod)) {
                        if (runCommand(command.value(), false)) {
                            break;
                        }
                    }

                    // Fallback to the focus target
                    switch (m_cursor_context.focus_target) {
                        case FocusTarget::Editor:
                            if (m_editor.onKeyDown(m_cursor_context, m_editor_state, event.key.keysym.sym, event.key.keysym.mod)) {
                                // If the view return true, then redraw the views
                                m_cursor_context.wants_redraw = true;
                            }
                        break;
                        case FocusTarget::Prompt:
                            if (m_prompt.onKeyDown(m_cursor_context, m_prompt_state, event.key.keysym.sym, event.key.keysym.mod)) {
                                // If the view return true, then redraw the views
                                m_cursor_context.wants_redraw = true;
                            }
                        break;
                    }
                }
                break;
                case SDL_TEXTINPUT: {
                    // Don't process key input if of these modifiers are held
                    const auto block_text_input = SDL_GetModState() & KMOD_CTRL; // KMOD_SHIFT / KMOD_ALT ?
                    if (!block_text_input) {
                        // Redirect to input focus. We always redraw new characters.
                        m_cursor_context.wants_redraw = true;
                        switch (m_cursor_context.focus_target) {
                            case FocusTarget::Editor:
                                m_editor.onTextInput(m_cursor_context, m_editor_state, event.text.text);
                                break;
                            case FocusTarget::Prompt:
                                m_prompt.onTextInput(m_cursor_context, m_prompt_state, event.text.text);
                                break;
                        }
                    }
                }
                break;
                case SDL_MOUSEWHEEL: {
                    // We must have an updated value for the line_height, so request the size from the theme now
                    const auto line_height = m_theme.getLineHeight();
                    const auto scroll_amount = event.wheel.y * -line_height;
                    m_cursor_context.scroll_y = m_cursor_context.scroll_y + scroll_amount;
                    m_cursor_context.wants_redraw = true;
                }
                break;
                default:
                break;
            }
        }

        // Calculate dt time
        const auto currentTime = SDL_GetPerformanceCounter();
        const auto dt = static_cast<float>(currentTime - lastTime) / performanceQuery;
        lastTime = currentTime;
        if (m_cursor_context.wants_redraw) {
            // Need to redraw the whole views
            const auto border_size = m_theme.getDimension(DimensionId::BorderSize);
            const auto line_height = m_theme.getLineHeight();
            const auto bar_height = static_cast<int16_t>(line_height + border_size);
            const auto bar_width = static_cast<int16_t>(window_width);

            m_info_bar_state.setPosition(0, 0);
            m_info_bar_state.setSize(bar_width, bar_height);

            m_prompt_state.setPosition(0, static_cast<int16_t>(window_height - bar_height));
            m_prompt_state.setSize(bar_width, bar_height);

            m_editor_state.setPosition(0, bar_height);
            m_editor_state.setSize(bar_width, static_cast<uint16_t>(window_height - bar_height * 2));

            glViewport(0, 0, window_width, window_height);
            glScissor(0, 0, window_width, window_height);
            glClearColor(0.0f, 0.0, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // Render everything on screen.
            m_cursor_context.highlighter.parse();
            m_info_bar.render(m_cursor_context, m_info_bar_state, dt);
            m_editor.render(m_cursor_context, m_editor_state, dt);
            m_prompt.render(m_cursor_context, m_prompt_state, dt);

            // todo: Uncomment for debug purpose.
            // std::cout << "view updated " << std::endl;
            m_cursor_context.wants_redraw = false;
            if (m_prompt_state.getRunningState() == PromptState::RunningState::Message) {
                // If the prompt show a message, reset the state now to
                // clear it and display the PROMPT_READY message when the next frame refreshes.
                m_prompt_state.setRunningState(PromptState::RunningState::Idle);
                m_prompt_state.setPromptText(PromptState::PROMPT_READY);
            }
        }

        // Reset follow_indicator if it was not held by the editor render already
        m_cursor_context.follow_indicator = false;

        // Update max_render_time metrics
        const auto frame_time_elapsed = static_cast<float>(SDL_GetPerformanceCounter() - frame_time) / performanceQuery;
        if (frame_time_elapsed > m_render_time->m_value) {
            m_render_time->m_value = frame_time_elapsed;
        }

        SDL_GL_SwapWindow(p_sdl_window);
    }
}

void ApplicationWindow::getCommandCompletions(const std::string_view input, const AutoCompleteCallback<char> &itemCallback) {
    m_command_manager.getCommandCompletions(input, itemCallback);
}

void ApplicationWindow::getArgumentsCompletions(const std::string_view command, const int32_t argumentIndex, const std::string_view input, const AutoCompleteCallback<char> &itemCallback) {
    m_command_manager.getArgumentsCompletion(command, argumentIndex, input, itemCallback);
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

bool ApplicationWindow::runCommand(const std::u16string_view command, const bool fromPrompt) {
    std::optional<std::u16string> result;
    if (m_cursor_context.command_feedback) {
        // Before executing the command, check if feedback exists
        // Copy the feedback object so the string is still valid after reset is called.
        const auto feedback = m_cursor_context.command_feedback.value();
        const auto feedback_answer = m_prompt_cursor.getString();
        const auto tokens = CommandManager::tokenize(feedback_answer);
        m_cursor_context.command_feedback.reset();
        if (tokens.size() == 1) {
            // For now only support 1 token from feedback answers
            m_prompt_state.setRunningState(PromptState::RunningState::Validated);
            result = feedback.on_validate_callback(tokens[0], feedback.command_string);
        } else {
            // If we got no response from feedback, make it idle.
            m_prompt_state.setRunningState(PromptState::RunningState::Idle);
        }
    } else {
        const auto &tokens = CommandManager::tokenize(command);
        if (tokens.empty()) {
            return false;
        }

        const auto utf8_command_name = utf8::utf16to8(tokens[0]);
        const auto allowed_to_run = m_command_manager.isRunnable(m_cursor_context, utf8_command_name);
        if (!allowed_to_run) {
            // Nothing to process
            return false;
        }

        if (fromPrompt) {
            // If a command is not running from direct prompt input, don't add it to history
            m_prompt_state.addHistory(command);

            // Move focus to the editor if we run this command from the prompt,
            // because we don't want the next command to apply in the prompt in this case (e.g: "move up").
            m_cursor_context.focus_target = FocusTarget::Editor;
        }

        result = m_command_manager.run(m_cursor_context, tokens);
    }

    if (result) {
        // Show the error message in the prompt, if any.
        m_prompt_state.setRunningState(PromptState::RunningState::Message);
        m_prompt_state.setPromptText(*result);
        m_prompt_state.clearCompletions();
        m_prompt_state.clearHistoryIndex();
        m_prompt_cursor.clear();
        m_cursor_context.wants_redraw = true;

        // Focus go to the editor
        m_cursor_context.focus_target = FocusTarget::Editor;
    } else {
        // After command execution, we need to know if feedback is available,
        // so query the feedback again.
        if (m_cursor_context.command_feedback) {
            // The prompt needs a feedback, so let it running and update the
            // prompt text with the feedback
            m_prompt_state.setRunningState(PromptState::RunningState::Running);
            m_prompt_state.setPromptText(m_cursor_context.command_feedback->prompt_message);
            m_prompt_state.clearCompletions();
            m_prompt_state.clearHistoryIndex();
            m_prompt_cursor.clear();
            m_cursor_context.wants_redraw = true;

            // Focus go to the prompt
            m_cursor_context.focus_target = FocusTarget::Prompt;
        } else {
            // The prompt state can change while command execution (e.g: activate_prompt, cancel), check it again.
            const auto prompt_state = m_prompt_state.getRunningState();
            switch (prompt_state) {
                case PromptState::RunningState::Validated:
                    m_prompt_state.setRunningState(PromptState::RunningState::Idle);
                case PromptState::RunningState::Idle:
                    m_cursor_context.command_feedback.reset();
                    m_prompt_state.clearCompletions();
                    m_prompt_state.clearHistoryIndex();
                    m_prompt_state.setPromptText(PromptState::PROMPT_READY);
                    m_prompt_cursor.clear();
                    m_cursor_context.wants_redraw = true;

                    m_cursor_context.focus_target = FocusTarget::Editor;
                break;
                default:
                    // Don't change anything
                break;
            }
        }
    }

    return true;
}

void ApplicationWindow::getFeedbackCompletions(const AutoCompleteCallback<char16_t> &itemCallback) const {
    if (m_cursor_context.command_feedback) {
        for (const auto &completion : m_cursor_context.command_feedback->completions_list) {
            itemCallback(completion);
        }
    }
}

