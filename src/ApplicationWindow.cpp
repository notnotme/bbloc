#include "ApplicationWindow.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>

#include <SDL_image.h>
#include <glad/glad.h>
#include <utf8.h>

#include "core/cursor/buffer/StringBuffer.h"
#include "core/cursor/buffer/VectorBuffer.h"
#include "core/theme/DimensionId.h"


ApplicationWindow::ApplicationWindow()
    : p_sdl_window(nullptr),
      m_sdl_gl_context(nullptr),
      m_render_time(std::make_shared<CVarFloat>(0.0f, true)),
      m_cursor_context(m_prompt_cursor, std::make_unique<VectorBuffer>()),
      m_info_bar(m_command_manager, m_theme, m_quad_program, m_quad_buffer),
      m_editor(m_command_manager, m_theme, m_quad_program, m_quad_buffer),
      m_prompt_state(m_command_manager),
      m_prompt(m_command_manager, m_theme, m_quad_program, m_quad_buffer),
      m_focus_target(FocusTarget::Editor),
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

    // Register commands with the command manager and run autoexec
    registerQuitCommand();
    registerOpenCommand();
    registerSaveCommand();
    registerBindCommand();
    registerRenderTimeCommand();
    registerFontSizeCommand();
    registerActivatePromptCommand();
    registerCopyCommand();
    registerPasteCommand();
    registerCutCommand();
    registerHighlighterCommand();
    registerExecCommand();
    registerMoveCommands();
    registerCancelCommand();
    registerValidateCommand();
    registerAutoCompleteCommand();
    m_command_manager.execute(m_cursor_context, { u"exec", utf8::utf8to16(path).append(u"autoexec") });
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
                    const auto modifiers = normalizeModifiers(event.key.keysym.mod);
                    if (const auto &map_entry = m_key_bindings.find(event.key.keysym.sym); map_entry != m_key_bindings.end()) {
                        if (const auto &binding = map_entry->second.find(modifiers); binding != map_entry->second.end()) {
                            // This does not necessarily redraw here, let the runCommand decide.
                            if (runCommand(binding->second, false)) {
                                break;
                            }
                        }
                    }

                    // Fallback to the focus target
                    switch (m_focus_target) {
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
                        switch (m_focus_target) {
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
    if (m_command_feedback) {
        // Before executing the command, check if feedback exists
        // Copy the feedback object so the string is still valid after reset is called.
        const auto feedback = m_command_feedback.value();
        const auto feedback_answer = m_prompt_cursor.getString();
        const auto tokens = CommandManager::tokenize(feedback_answer);
        m_command_feedback.reset();
        if (tokens.size() == 1) {
            // For now only support 1 token from feedback answers
            m_prompt_state.setRunningState(PromptState::RunningState::Validated);
            result = feedback.on_validate_callback(tokens[0], feedback.command);
        } else {
            // If we got no response from feedback, make it idle.
            m_prompt_state.setRunningState(PromptState::RunningState::Idle);
        }
    } else {
        const auto &tokens = CommandManager::tokenize(command);
        const auto allowed_to_run = m_command_manager.canExecute(tokens);
        if (tokens.empty() || !allowed_to_run) {
            // Nothing to process
            return false;
        }

        if (fromPrompt) {
            // If a command is not running from direct prompt input, don't add it to history
            m_prompt_state.addHistory(command);

            // Move focus to the editor if we run this command from the prompt,
            // because we don't want the next command to apply in the prompt in this case (e.g: "move up").
            m_focus_target = FocusTarget::Editor;
        }

        result = m_command_manager.execute(m_cursor_context, tokens);
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
        m_focus_target = FocusTarget::Editor;
    } else {
        // After command execution, we need to know if feedback is available,
        // so query the feedback again.
        if (m_command_feedback) {
            // The prompt needs a feedback, so let it running and update the
            // prompt text with the feedback
            m_prompt_state.setRunningState(PromptState::RunningState::Running);
            m_prompt_state.setPromptText(m_command_feedback->prompt);
            m_prompt_state.clearCompletions();
            m_prompt_state.clearHistoryIndex();
            m_prompt_cursor.clear();
            m_cursor_context.wants_redraw = true;

            // Focus go to the prompt
            m_focus_target = FocusTarget::Prompt;
        } else {
            // The prompt state can change while command execution (e.g: activate_prompt, cancel), check it again.
            const auto prompt_state = m_prompt_state.getRunningState();
            switch (prompt_state) {
                case PromptState::RunningState::Validated:
                    m_prompt_state.setRunningState(PromptState::RunningState::Idle);
                case PromptState::RunningState::Idle:
                    m_command_feedback.reset();
                    m_prompt_state.clearCompletions();
                    m_prompt_state.clearHistoryIndex();
                    m_prompt_state.setPromptText(PromptState::PROMPT_READY);
                    m_prompt_cursor.clear();
                    m_cursor_context.wants_redraw = true;

                    m_focus_target = FocusTarget::Editor;
                break;
                default:
                    // Don't change anything
                break;
            }
        }
    }

    return true;
}

void ApplicationWindow::getFeedbackCompletion(const ItemCallback<char16_t> &itemCallback) const {
    if (m_command_feedback) {
        for (const auto &completion : m_command_feedback->completions) {
            itemCallback(completion);
        }
    }
}

uint16_t ApplicationWindow::normalizeModifiers(const uint16_t modifiers) {
    uint16_t result = 0;
    if (modifiers & (KMOD_LCTRL | KMOD_RCTRL)) {
        result |= KMOD_CTRL;
    }

    if (modifiers & (KMOD_LSHIFT | KMOD_RSHIFT)) {
        result |= KMOD_SHIFT;
    }

    if (modifiers & (KMOD_LALT | KMOD_RALT)) {
        result |= KMOD_ALT;
    }

    if (modifiers & (KMOD_LGUI | KMOD_RGUI)) {
        result |= KMOD_GUI;
    }

    return result;
}

void ApplicationWindow::registerOpenCommand() {
    // Add the "open" command to open files and populate Cursor's buffer
    m_command_manager.registerCommand("open",
        [](const std::vector<std::u16string_view> &commandTokens) {
            (void) commandTokens;
            // This can be run with no restriction
            return true;
        },
        [&](CursorContext &context, const std::vector<std::u16string_view> &args) -> std::optional<std::u16string> {
            if (args.size() != 1) {
                return u"Usage: open <filename>";
            }

            const auto path = utf8::utf16to8(args[0]);
            const auto is_regular_file = std::filesystem::is_regular_file(path);
            auto ifs = std::ifstream(path, std::ios::in);
            if (!ifs || !ifs.is_open() || !is_regular_file) {
                // That file cannot be opened
                return std::u16string(u"Could not open ").append(args[0]).append(u".");
            }

            // Clear the cursor and read the file line by line
            const auto &edit_clear = context.cursor.clear();
            context.highlighter.edit(edit_clear);

            const auto file_extension = std::filesystem::path(path).extension().string();
            context.highlighter.setMode(file_extension);

            auto line_count = 1u;
            auto line = std::string();
            auto all_line = std::u16string();
            while (getline(ifs, line)) {
                if (const auto &end_it = utf8::find_invalid(line.begin(), line.end()); end_it != line.end()) {
                    // Invalid sequence: stop reading the file
                    const auto utf16_line_count = utf8::utf8to16(std::to_string(line_count));
                    return std::u16string(u"Invalid UTF-8 encoding detected at line ").append(utf16_line_count);
                }
                // Convert to utf16 then append to the cursor
                all_line.append(utf8::utf8to16(line));
                if (!ifs.eof() && !ifs.fail()) {
                    // After the first insert, line ends with \n, but not the last
                    all_line.append(u"\n");
                }

                ++line_count;
            }
            ifs.close();

            const auto &edit_insert = context.cursor.insert(all_line);
            context.highlighter.edit(edit_insert);

            context.cursor.setName(path);
            context.cursor.setPosition(0, 0);
            context.follow_indicator = true;

            // In case the command is bound to a key, it will needs a redraw the views.
            // Otherwise, when the user type enter wants_redraw is set to true when processing the events.
            context.wants_redraw = true;
            return std::nullopt;
        },
        [&](const CursorContext &context, const int32_t argumentIndex, const std::string_view input, const ItemCallback<char> &itemCallback) {
            (void) context;
            if (argumentIndex != 0) {
                // Only auto-complete the first argument (path)
                return;
            }
            CommandManager::getPathCompletions(input, false, itemCallback);
        });
}

void ApplicationWindow::registerSaveCommand() {
    // Add the "save" command to save files to disk
    m_command_manager.registerCommand("save",
        [](const std::vector<std::u16string_view> &commandTokens) {
            (void) commandTokens;
            // This can be run with no restriction
            return true;
        },
        [&](CursorContext &context, const std::vector<std::u16string_view> &args) -> std::optional<std::u16string> {
            const auto cursor_name = std::filesystem::path(context.cursor.getName());
            if (cursor_name.empty() && (args.empty() || (args.size() >= 2 && args[1] != u"-f"))) {
                return u"Usage: save <filename> [-f]";
            }

            const auto arg_filename = std::filesystem::path(args.empty() ? "" : utf8::utf16to8(args[0]));
            const auto file_to_save = arg_filename.empty() ? cursor_name : arg_filename;
            const auto file_exists = std::filesystem::exists(file_to_save);
            const auto is_regular_file = std::filesystem::is_regular_file(file_to_save);
            if (file_exists && !is_regular_file) {
                return std::u16string(u"Could not save ").append(args[0]).append(u".");
            }

            if (cursor_name.filename() != file_to_save.filename()
                && file_exists
                && (args.size() == 1 || args[1] != u"-f")) {
                // The file already exists and is not the cursor one, needs user feedback to be able to overwrite it
                m_command_feedback = CommandFeedback {
                    .prompt = u"File already exists, overwrite ? [y/N]:",
                    .command = std::u16string(u"save ").append(args[0]).append(u" -f"),
                    .completions = {u"n", u"y"},
                    .on_validate_callback = [&](const std::u16string_view answer, const std::u16string_view command) -> std::optional<std::u16string> {
                        if (answer == u"y" || answer == u"Y") {
                            runCommand(command, true);
                            return std::nullopt;
                        }
                        return std::nullopt;
                    }
                };

                return std::nullopt;
            }

            auto ofs = std::ofstream(file_to_save, std::ios::out);
            if(!ofs || !ofs.is_open()) {
                return std::u16string(u"Could not save ").append(args[0]).append(u".");
            }

            const auto line_count = context.cursor.getLineCount();
            for (auto line = 0; line < line_count; ++line) {
                const auto string = context.cursor.getString(line);
                ofs << utf8::utf16to8(string);
                if(line < line_count - 1) {
                    ofs << "\n";
                }
            }

            ofs.close();
            context.cursor.setName(file_to_save.string());

            // We always want to redraw, in case we do not run from a prompt confirmation (bound command)
            context.wants_redraw = true;
            return std::nullopt;
        },
        [&](const CursorContext &context, const int32_t argumentIndex, const std::string_view input, const ItemCallback<char> &itemCallback) {
            (void) context;
            if (argumentIndex != 0) {
                // Only auto-complete the first argument (path)
                return;
            }
            CommandManager::getPathCompletions(input, false, itemCallback);
        });
}

void ApplicationWindow::registerRenderTimeCommand() {
    // Register inf_render_time cvar and associated command to reset the value, since we made it read-only
    m_command_manager.registerCvar("inf_render_time", m_render_time);
    m_command_manager.registerCommand("reset_render_time",
        [](const std::vector<std::u16string_view> &commandTokens) {
            (void) commandTokens;
            // This can be run with no restriction
            return true;
        },
        [&](const CursorContext &context, const std::vector<std::u16string_view> &args) -> std::optional<std::u16string> {
            (void) context;
            if (!args.empty()) {
                return u"Expected 0 argument.";
            }

            m_render_time->m_value = 0.0f;
            return std::nullopt;
        });
}

void ApplicationWindow::registerQuitCommand() {
    // Register quit command
    m_command_manager.registerCommand("quit",
        [](const std::vector<std::u16string_view> &commandTokens) {
            (void) commandTokens;
            // This can be run with no restriction
            return true;
        },
        [&](const CursorContext &context, const std::vector<std::u16string_view> &args) -> std::optional<std::u16string> {
            (void) context;
            if (!args.empty()) {
                return u"Expected 0 argument.";
            }

            SDL_Event event { .type = SDL_QUIT };
            SDL_PushEvent(&event);
            return std::nullopt;
        });
}

void ApplicationWindow::registerBindCommand() {
    // Register bind command
    m_command_manager.registerCommand("bind",
        [](const std::vector<std::u16string_view> &commandTokens) {
            (void) commandTokens;
            // This can be run with no restriction
            return true;
        },
        [&](const CursorContext &context, const std::vector<std::u16string_view> &args) -> std::optional<std::u16string> {
            (void) context;
            if (args.size() < 3 || (!args.empty() && args[1].empty())) {
                return u"Usage: bind <modifiers> <key> <command>";
            }

            const auto split_modifiers = CommandManager::split(args[0], u'+');

            auto modifier = 0;
            for (const auto &string_modifier : split_modifiers) {
                if (string_modifier == u"Ctrl") {
                    modifier |= KMOD_CTRL;
                } else if (string_modifier == u"Alt") {
                    modifier |= KMOD_ALT;
                } else if (string_modifier == u"Shift") {
                    modifier |= KMOD_SHIFT;
                } else if (string_modifier == u"None") {
                    // Dummy op
                } else {
                    return std::u16string(u"Unknown modifier: ").append(string_modifier);
                }
            }

            const auto keycode_utf8 = utf8::utf16to8(args[1]);
            const auto key = SDL_GetKeyFromName(keycode_utf8.data());
            if (! m_key_bindings.contains(key)) {
                m_key_bindings.emplace(key, std::unordered_map<uint16_t, std::u16string>());
            }

            m_key_bindings.at(key).insert_or_assign(modifier, args[2]);
            return std::nullopt;
        });
}

void ApplicationWindow::registerFontSizeCommand() {
    // Register increase_font_size and decrease_font_size command
    m_command_manager.registerCommand("increase_font_size",
        [](const std::vector<std::u16string_view> &commandTokens) {
            (void) commandTokens;
            // This can be run with no restriction
            return true;
        },
        [&](CursorContext &context, const std::vector<std::u16string_view> &args) -> std::optional<std::u16string> {
            (void) context;
            if (!args.empty()) {
                return u"Expected 0 argument.";
            }

            const auto font_size = m_theme.getFontSize();
            m_theme.setFontSize(font_size + 1);
            context.wants_redraw = true;

            return std::nullopt;
        });

    m_command_manager.registerCommand("decrease_font_size",
        [](const std::vector<std::u16string_view> &commandTokens) {
            (void) commandTokens;
            // This can be run with no restriction
            return true;
        },
        [&](CursorContext &context, const std::vector<std::u16string_view> &args) -> std::optional<std::u16string> {
            (void) context;
            if (!args.empty()) {
                return u"Expected 0 argument.";
            }

            const auto font_size = m_theme.getFontSize();
            m_theme.setFontSize(font_size - 1);
            context.wants_redraw = true;

            return std::nullopt;
        });
}

void ApplicationWindow::registerActivatePromptCommand() {
    // Register activate_prompt command
    m_command_manager.registerCommand("activate_prompt",
        [&](const std::vector<std::u16string_view> &commandTokens) {
            (void) commandTokens;
            // This can be run only if the editor has the focus
            return m_focus_target == FocusTarget::Editor;
        },
        [&](CursorContext &context, const std::vector<std::u16string_view> &args) -> std::optional<std::u16string> {
            (void) context;
            if (!args.empty()) {
                return u"Expected 0 argument.";
            }

            // Set focus to prompt (since the editor had it if we run from a binding)
            m_focus_target = FocusTarget::Prompt;
            // Set prompt to running state
            m_prompt_state.setRunningState(PromptState::RunningState::Running);
            m_prompt_state.setPromptText(PromptState::PROMPT_ACTIVE);
            m_prompt_cursor.clear();

            context.wants_redraw = true;
            return std::nullopt;
        });
}

void ApplicationWindow::registerCopyCommand() {
    // Resisters the copy command
    m_command_manager.registerCommand("copy",
        [&](const std::vector<std::u16string_view> &commandTokens) {
            (void) commandTokens;
            // This can be run only if the editor has the focus
            return m_focus_target == FocusTarget::Editor;
        },
        [&](const CursorContext &context, const std::vector<std::u16string_view> &args) -> std::optional<std::u16string> {
            if (!args.empty()) {
                return u"Expected 0 argument.";
            }

            const auto &selection = context.cursor.getSelectedText();
            if (!selection) {
                return u"Selection is empty.";
            }

            auto to_clipboard_text = std::u16string();
            const auto &all_text = selection.value();
            const auto all_text_size = all_text.size();
            for (auto i = 0; i < all_text_size; ++i) {
                to_clipboard_text = to_clipboard_text.append(all_text[i]);
                if (i < all_text_size - 1) {
                    to_clipboard_text = to_clipboard_text.append(u"\n");
                }
            }

            const auto utf8_clipboard_text = utf8::utf16to8(to_clipboard_text);
            SDL_SetClipboardText(utf8_clipboard_text.data());

            return std::nullopt;
        });
}

void ApplicationWindow::registerPasteCommand() {
    // Resisters the paste command
    m_command_manager.registerCommand("paste",
        [&](const std::vector<std::u16string_view> &commandTokens) {
            (void) commandTokens;
            // This can be run only if the editor has the focus
            return m_focus_target == FocusTarget::Editor;
        },
        [&](CursorContext &context, const std::vector<std::u16string_view> &args) -> std::optional<std::u16string> {
            if (!args.empty()) {
                return u"Expected 0 argument.";
            }

            char *sdl_clipboard_text = SDL_GetClipboardText();
            const auto clipboard_text = std::string(sdl_clipboard_text);
            const auto utf16_clipboard_text = utf8::utf8to16(clipboard_text);
            if (utf16_clipboard_text.empty()) {
                SDL_free(sdl_clipboard_text);
                return u"Clipboard is empty.";
            }

            if (context.cursor.getSelectedRange()) {
                const auto &edit = context.cursor.eraseSelection();
                context.highlighter.edit(edit.value());
            }

            const auto &edit = context.cursor.insert(utf16_clipboard_text);
            context.highlighter.edit(edit);
            context.follow_indicator = true;

            SDL_free(sdl_clipboard_text);
            context.cursor.activateSelection(false);

            context.wants_redraw = true;
            return std::nullopt;
        });
}

void ApplicationWindow::registerCutCommand() {
    // Resisters the cur command
    m_command_manager.registerCommand("cut",
        [&](const std::vector<std::u16string_view> &commandTokens) {
            (void) commandTokens;
            // This can be run only if the editor has the focus
            return m_focus_target == FocusTarget::Editor;
        },
        [&](CursorContext &context, const std::vector<std::u16string_view> &args) -> std::optional<std::u16string> {
            if (!args.empty()) {
                return u"Expected 0 argument.";
            }

            const auto &selection = context.cursor.getSelectedText();
            if (!selection) {
                return u"Selection is empty.";
            }

            auto to_clipboard_text = std::u16string();
            const auto &all_text = selection.value();
            const auto all_text_size = all_text.size();
            for (auto i = 0; i < all_text_size; ++i) {
                to_clipboard_text = to_clipboard_text.append(all_text[i]);
                if (i < all_text_size - 1) {
                    to_clipboard_text = to_clipboard_text.append(u"\n");
                }
            }

            if (const auto &edit = context.cursor.eraseSelection()) {
                context.highlighter.edit(edit.value());
                context.cursor.activateSelection(false);
            }

            const auto utf8_clipboard_text = utf8::utf16to8(to_clipboard_text);
            SDL_SetClipboardText(utf8_clipboard_text.data());

            context.wants_redraw = true;
            return std::nullopt;
        });
}

void ApplicationWindow::registerHighlighterCommand() {
    // Register a command to change the highlight mode
    m_command_manager.registerCommand("set_hl_mode",
        [](const std::vector<std::u16string_view> &commandTokens) {
            (void) commandTokens;
            // This can be run with no restriction
            return true;
        },
        [&](CursorContext &context, const std::vector<std::u16string_view> &args) -> std::optional<std::u16string> {
            if (args.size() != 1) {
                return u"Usage: set_hl_mode <mode>";
            }

            // Developers are lazy, so let's prepend a dot to make it work flawlessly
            const auto extension = std::string(".").append(utf8::utf16to8(args[0]));
            if (!HighLighter::isSupported(extension)) {
                return std::u16string(u"Unsupported highlight mode: ").append(args[0]);
            }

            context.highlighter.setMode(extension);
            context.wants_redraw = true;
            return std::nullopt;
        },
        [&](const CursorContext &context, const int32_t argumentIndex, const std::string_view input, const ItemCallback<char> &itemCallback) {
            // Ignore input
            (void) context;
            (void) input;
            if (argumentIndex != 0) {
                // Only auto-complete the first argument (mode)
                return;
            }

            HighLighter::getParserNames(itemCallback);
        });
}

void ApplicationWindow::registerExecCommand() {
    // Register the exec command, that read a text file on disk and execute each line as command
    m_command_manager.registerCommand("exec",
        [](const std::vector<std::u16string_view> &commandTokens) {
            (void) commandTokens;
            // This can be run with no restriction
            return true;
        },
        [&](CursorContext &context, const std::vector<std::u16string_view> &args) -> std::optional<std::u16string> {
            if (args.size() != 1) {
                return u"Usage: exec <filename>";
            }

            const auto path = utf8::utf16to8(args[0]);
            const auto is_regular_file = std::filesystem::is_regular_file(path);
            auto ifs = std::ifstream(path, std::ios::in);
            if (!ifs || !ifs.is_open() || !is_regular_file) {
                // That file cannot be opened
                return std::u16string(u"Could not open ").append(args[0]).append(u".");
            }

            auto command_list = std::vector<std::u16string>();
            auto line_count = 1;
            auto line = std::string();
            while (getline(ifs, line)) {
                if (const auto &end_it = utf8::find_invalid(line.begin(), line.end()); end_it != line.end()) {
                    // Invalid sequence: stop the command list
                    const auto utf16_line_count = utf8::utf8to16(std::to_string(line_count));
                    return std::u16string(u"Invalid UTF-8 encoding detected at line ").append(utf16_line_count);
                }

                // Convert to utf16 then append to the cursor
                if (!line.starts_with("#")) {
                    const auto u16string = utf8::utf8to16(line);
                    command_list.emplace_back(u16string);
                }
                ++line_count;
            }
            ifs.close();

            for (const auto &command : command_list) {
                // fixme?: At this point, any feedback needed will interrupt the command list execution
                // fixme?: autoexec does not show in history
                // todo: take in account "#" as comment and don't execute the line
                const auto &tokens = CommandManager::tokenize(command);
                if (const auto &result = m_command_manager.execute(context, tokens)) {
                    context.wants_redraw = true;
                    return result.value();
                }
            }

            return std::nullopt;
    },
    [&](const CursorContext &context, const int32_t argumentIndex, const std::string_view input, const ItemCallback<char> &itemCallback) {
        (void) context;
       if (argumentIndex != 0) {
           // Only auto-complete the first argument (path)
           return;
       }
       CommandManager::getPathCompletions(input, false, itemCallback);
   });
}

void ApplicationWindow::registerMoveCommands() {
    // Register the move commands
    m_command_manager.registerCommand("move",
        [](const std::vector<std::u16string_view> &commandTokens) {
            (void) commandTokens;
            // This can be run with no restriction
            return true;
        },
        [&](CursorContext &context, const std::vector<std::u16string_view> &args) -> std::optional<std::u16string> {
            if (args.empty() || args.size() > 2) {
                return u"Usage: move <direction> [selected]";
            }

            int direction;
            if (args[0] == u"up") {
                direction = 0;
            } else if (args[0] == u"down") {
                direction = 1;
            } else if (args[0] == u"left") {
                direction = 2;
            } else if (args[0] == u"right") {
                direction = 3;
            } else if (args[0] == u"bol") {
                direction = 4;
            } else if (args[0] == u"eol") {
                direction = 5;
            } else if (args[0] == u"page_up") {
                direction = 6;
            } else if (args[0] == u"page_down") {
                direction = 7;
            }  else if (args[0] == u"eof") {
                direction = 8;
            } else if (args[0] == u"bof") {
                direction = 9;
            } else {
                return std::u16string(u"Unknown direction argument: ").append(args[0]);
            }

            bool selected;
            if (args.size() == 2) {
                if (args[1] == u"true") {
                    selected = true;
                } else if (args[1] == u"false") {
                    selected = false;
                } else {
                    return std::u16string(u"Selected argument expect a boolean value: ").append(args[1]);
                }
            } else {
                selected = false;
            }

            switch (m_focus_target) {
                case FocusTarget::Prompt: {
                    switch (direction) {
                        case 0: {
                            if (m_prompt_state.getHistoryCount() > 0) {
                                m_prompt_state.clearCompletions();
                                const auto command = m_prompt_state.previousHistory();
                                context.prompt_cursor.clear();
                                context.prompt_cursor.insert(command);
                                context.wants_redraw = true;
                            }
                            break;
                        }
                        case 1: {
                            if (m_prompt_state.getHistoryCount() > 0) {
                                m_prompt_state.clearCompletions();
                                const auto command = m_prompt_state.nextHistory();
                                context.prompt_cursor.clear();
                                context.prompt_cursor.insert(command);
                                context.wants_redraw = true;
                            }
                            break;
                        }
                        case 2:
                            context.prompt_cursor.moveLeft();
                            context.wants_redraw = true;
                        break;
                        case 3:
                            context.prompt_cursor.moveRight();
                            context.wants_redraw = true;
                        break;
                        case 4:
                            context.prompt_cursor.moveToStart();
                            context.wants_redraw = true;
                        break;
                        case 5:
                            context.prompt_cursor.moveToEnd();
                            context.wants_redraw = true;
                        break;
                        default:
                            // No-op
                        break;
                    }

                    return std::nullopt;
                }
                case FocusTarget::Editor: {
                    context.cursor.activateSelection(selected);
                    switch (direction) {
                        default:
                        case 0: context.cursor.moveUp(); break;
                        case 1: context.cursor.moveDown(); break;
                        case 2: context.cursor.moveLeft(); break;
                        case 3: context.cursor.moveRight(); break;
                        case 4: context.cursor.moveToStartOfLine(); break;
                        case 5: context.cursor.moveToEndOfLine(); break;
                        case 6: context.cursor.pageUp(m_theme.getDimension(DimensionId::PageUpDown)); break;
                        case 7: context.cursor.pageDown(m_theme.getDimension(DimensionId::PageUpDown)); break;
                        case 8: context.cursor.moveToStartOfFile(); break;
                        case 9: context.cursor.moveToEndOfFile(); break;
                    }

                    context.wants_redraw = true;
                    context.follow_indicator = true;
                    return std::nullopt;
                }
                default:
                return std::nullopt;
            }
        });
}

void ApplicationWindow::registerCancelCommand() {
    // Register cancel command
    m_command_manager.registerCommand("cancel",
        [&](const std::vector<std::u16string_view> &commandTokens) {
            (void) commandTokens;
            // Only the prompt can be canceled
            return m_focus_target == FocusTarget::Prompt;
        },
        [&](const CursorContext &context, const std::vector<std::u16string_view> &args) -> std::optional<std::u16string> {
            (void) context;
            if (!args.empty()) {
                return u"Expected 0 argument.";
            }

            switch (m_focus_target) {
                case FocusTarget::Prompt:
                    // Reset completions, history index, and feedback if the user quit the prompt
                    m_prompt_state.setRunningState(PromptState::RunningState::Idle);
                break;
                case FocusTarget::Editor:
                    // No-op
                default:
                break;
            }

            return std::nullopt;
        });
}

void ApplicationWindow::registerValidateCommand() {
    // Register validate command
    m_command_manager.registerCommand("validate",
        [&](const std::vector<std::u16string_view> &commandTokens) {
            (void) commandTokens;
            // Only the prompt can be validated
            return m_focus_target == FocusTarget::Prompt;
        },
        [&](CursorContext &context, const std::vector<std::u16string_view> &args) -> std::optional<std::u16string> {
            if (!args.empty()) {
                return u"Expected 0 argument.";
            }

            switch (m_focus_target) {
                case FocusTarget::Prompt: {
                    // If "validate" is ran via keybind while the prompt is active, then run the command in it.
                    m_prompt_state.setRunningState(PromptState::RunningState::Validated);
                    const auto prompt_command = m_prompt_cursor.getString();
                    runCommand(prompt_command, true);
                }
                break;
                case FocusTarget::Editor:
                    // No-op
                default:
                break;
            }

            return std::nullopt;
        });
}

void ApplicationWindow::registerAutoCompleteCommand() {
    // Register validate command
    m_command_manager.registerCommand("auto_complete",
        [&](const std::vector<std::u16string_view> &commandTokens) {
            (void) commandTokens;
            // Only the prompt can be auto completed
            return m_focus_target == FocusTarget::Prompt;
        },
        [&](CursorContext &context, const std::vector<std::u16string_view> &args) -> std::optional<std::u16string> {
            if (args.size() > 1) {
                return u"Expected 0 or 1 argument.";
            }

            int direction = 0;
            if (!args.empty()) {
                if (args[0] == u"forward") {
                    direction = 0;
                } else if (args[0] == u"backward") {
                    direction = 1;
                }
            }

            switch (m_focus_target) {
                case FocusTarget::Prompt: {
                    const auto input = context.prompt_cursor.getString();
                    const auto tokens = CommandManager::tokenize(input);
                    // Reset the history index if we were browsing
                    m_prompt_state.clearHistoryIndex();
                    if (m_prompt_state.getCompletionCount() > 0) {
                        // The viewState completion list is not empty, loop inside
                        const auto completion = direction == 0 ? m_prompt_state.nextCompletion() : m_prompt_state.previousCompletion();
                        context.prompt_cursor.clear();
                        context.prompt_cursor.insert(completion);
                        context.wants_redraw = true;
                        return std::nullopt;
                    }

                    if (m_command_feedback) {
                        // If a feedback is active, try to gather arguments
                        getFeedbackCompletion([&](const std::u16string_view completion) {
                            m_prompt_state.addCompletion(completion);
                        });
                    } else {
                        // Find command name, argument, and argument index from the user input
                        const auto utf8_command_name = tokens.empty() ? "" : utf8::utf16to8(tokens.front());
                        const auto utf8_argument_to_complete = tokens.size() <= 1 ? "" : utf8::utf16to8(tokens.back());
                        const auto argument_index = std::max(0, static_cast<int32_t>(tokens.size() - 2));

                        // Try to complete commands arguments first, if the command name is incomplete, this will return an empty list
                        m_command_manager.getArgumentsCompletion(context, utf8_command_name, argument_index, utf8_argument_to_complete,
                            [&](const std::string_view completion) {
                                const auto completion_str = std::u16string(tokens[0]).append(u" ").append(utf8::utf8to16(completion));
                                m_prompt_state.addCompletion(completion_str);
                            });

                        if (m_prompt_state.getCompletionCount() == 0 && tokens.size() <= 1) {
                            // Auto-complete commands names
                            m_command_manager.getCommandCompletions(utf8_command_name,
                                [&](const std::string_view completion) {
                                    m_prompt_state.addCompletion(utf8::utf8to16(completion));
                                });
                        }
                    }

                    // Populate the viewState list and insert the first item in the cursor
                    const auto completion_count = m_prompt_state.getCompletionCount();
                    if (completion_count > 0) {
                        m_prompt_state.sortCompletions();

                        const auto completion = m_prompt_state.getCurrentCompletion();
                        context.prompt_cursor.clear();
                        context.prompt_cursor.insert(completion);

                        if (completion_count == 1) {
                            // If we got only a single result, then append a space at the end of the command line
                            // and clear the actual completion, so the next argument completion can occur.
                            context.prompt_cursor.insert(u" ");
                            m_prompt_state.clearCompletions();
                        }

                        context.wants_redraw = true;
                        return std::nullopt;
                    }
                }
                break;
                case FocusTarget::Editor:
                default:
                    // No-op
                break;
            }

            return std::nullopt;
        });
}
