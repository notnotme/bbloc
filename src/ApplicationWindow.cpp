#include "ApplicationWindow.h"

#include <cmath>
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
      m_cursor(std::make_unique<VectorBuffer>()),
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

    // Create the command manager and register commands
    m_command_manager.create();
    registerQuitCommand();
    registerOpenCommand();
    registerSaveCommand();
    registerBindCommand();
    registerRenderTimeCommand();
    registerFontSizeCommand();
    registerActivatePromptCommand();

    // Create the theme and highlighter
    constexpr auto path = "romfs/";
    constexpr auto path_utf16 = u"romfs/";
    m_theme.create(m_command_manager, path);
    m_high_lighter.create(m_command_manager);
    m_high_lighter.setInput(m_cursor);

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

    // Run autoexec
    m_command_manager.execute(m_cursor, std::u16string(u"exec ").append(path_utf16).append(u"autoexec"));

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
}

void ApplicationWindow::mainLoop() {
    // Request performance query used to calculate dt time
    const auto performanceQuery = static_cast<float>(SDL_GetPerformanceFrequency());
    auto window_width = 0;
    auto window_height = 0;
    auto is_running = true;
    auto dirty_flags = 0 | Views;
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
                            dirty_flags = Matrix | Views;
                            window_width = event.window.data1;
                            window_height = event.window.data2;
                        break;
                        default:
                        break;
                    }
                break;
                case SDL_KEYDOWN:
                    // Only run binding when the editor is focused (The prompt is not running then)
                    if (m_focus_target == FocusTarget::Editor) {
                        const auto modifiers = normalizeModifiers(event.key.keysym.mod);
                        if (const auto &map_entry = m_key_bindings.find(event.key.keysym.sym); map_entry != m_key_bindings.end()) {
                            if (const auto &binding = map_entry->second.find(modifiers); binding != map_entry->second.end()) {
                                runCommand(binding->second);
                                dirty_flags |= Views;
                                break;
                            }
                        }
                    }

                    switch (m_focus_target) {
                        case FocusTarget::Editor:
                            dirty_flags |= m_editor.onKeyDown(m_high_lighter, m_cursor, m_editor_state, event.key.keysym.sym, event.key.keysym.mod);
                        break;
                        case FocusTarget::Prompt: {
                            // Make views dirty only if the prompt is returning true
                            dirty_flags |= m_prompt.onKeyDown(m_high_lighter, m_prompt_cursor, m_prompt_state, event.key.keysym.sym, event.key.keysym.mod);
                            switch (m_prompt_state.getRunningState()) {
                                case PromptState::RunningState::Idle:
                                    // The prompt was canceled
                                    m_focus_target = FocusTarget::Editor;
                                    m_prompt_cursor.clear();
                                    m_prompt_state.setPromptText(PromptState::PROMPT_READY);
                                break;
                                case PromptState::RunningState::Validated: {
                                    // The prompt was validated
                                    const auto command_string = m_prompt_cursor.getString();
                                    runCommand(command_string);
                                    // Clear the prompt cursor now
                                    m_prompt_cursor.clear();
                                }
                                default:
                                break;
                            }
                        }
                        break;
                    }
                break;
                case SDL_TEXTINPUT: {
                    // Don't process key input if of these modifiers are held
                    const auto has_modifier = SDL_GetModState() & (KMOD_CTRL); // KMOD_SHIFT / KMOD_ALT ?
                    if (!has_modifier) {
                        dirty_flags |= Views;
                        switch (m_focus_target) {
                            case FocusTarget::Editor:
                                m_editor.onTextInput(m_high_lighter, m_cursor, m_editor_state, event.text.text);
                                break;
                            case FocusTarget::Prompt:
                                m_prompt.onTextInput(m_high_lighter, m_prompt_cursor, m_prompt_state, event.text.text);
                                break;
                        }
                    }
                }
                break;
                case SDL_MOUSEWHEEL: {
                    dirty_flags |= Views;
                    // We must have an updated value for the line_height, so request the size from the theme now
                    const auto line_height = m_theme.getLineHeight();
                    const auto scroll_amount = event.wheel.y * -line_height;
                    m_editor_state.setScrollY(m_editor_state.getScrollY() + scroll_amount);
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
        if (dirty_flags != 0) {
            if (dirty_flags & Matrix) {
                // We need to refresh the view's viewport, and the orthogonal matrix for the shader.
                updateOrthogonal(window_width, window_height);
                m_quad_program.setMatrix(m_orthogonal.data());
                m_info_bar.resizeWindow(window_width, window_height);
                m_editor.resizeWindow(window_width, window_height);
                m_prompt.resizeWindow(window_width, window_height);
                dirty_flags &= ~Matrix;
            }

            // Layout is done outside the views
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

            // Render (todo: remove green background used to check pixel leakage in debug)
            glViewport(0, 0, window_width, window_height);
            glScissor(0, 0, window_width, window_height);
            glClearColor(0.0f, 1.0, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // Render everything on screen.
            m_high_lighter.parse();
            m_info_bar.render(m_high_lighter, m_cursor, m_info_bar_state, dt);
            m_editor.render(m_high_lighter, m_cursor, m_editor_state, dt);
            m_prompt.render(m_high_lighter, m_prompt_cursor, m_prompt_state, dt);

            // todo: Uncomment for debug purpose.
            // std::cout << "view updated " << std::endl;
            dirty_flags &= ~Views;
            if (m_prompt_state.getRunningState() == PromptState::RunningState::Message) {
                // If the prompt show a message, reset the state now to
                // clear it and display the new message when the next frame refreshes.
                m_prompt_state.setRunningState(PromptState::RunningState::Idle);
                m_prompt_state.setPromptText(PromptState::PROMPT_READY);
            }
        }

        // Reset follow_indicator if it was not held by the editor render
        m_editor_state.setFollowIndicator(false);

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
    m_high_lighter.destroy();
    m_theme.destroy();
    m_command_manager.destroy();

    // Exit SDL
    SDL_GL_DeleteContext(m_sdl_gl_context);
    SDL_DestroyWindow(p_sdl_window);
    SDL_Quit();

    // Default states
    m_sdl_gl_context = nullptr;
    p_sdl_window = nullptr;
    m_orthogonal = {};
}

void ApplicationWindow::runCommand(const std::u16string_view command) {
    if (!m_command_manager.isCommandFeedbackPresent()) {
        // Before command execution, we need to know if feedback is available
        // If we have it, we don't push the answer to the feedback to the history
        m_prompt_state.addHistory(command);
    }

    if (const auto &message = m_command_manager.execute(m_cursor, command)) {
        // Show the error message in the prompt, if any
        m_prompt_state.setRunningState(PromptState::RunningState::Message);
        m_prompt_state.setPromptText(message.value());
        m_focus_target = FocusTarget::Editor;
    } else {
        // After command execution, we need to know if feedback is available,
        // so query the feedback again.
        if (const auto &feedback = m_command_manager.getCommandFeedback()) {
            // The prompt needs a feedback, so let it running and update the
            // prompt text with the feedback
            m_prompt_state.setRunningState(PromptState::RunningState::Running);
            m_prompt_state.setPromptText(feedback.value());
        } else {
            // The prompt state can change while command execution (e.g: activate_prompt), check it again.
            const auto prompt_state = m_prompt_state.getRunningState();
            if (prompt_state == PromptState::RunningState::Validated) {
                // Set prompt to idle
                m_focus_target = FocusTarget::Editor;
                m_prompt_state.setRunningState(PromptState::RunningState::Idle);
                m_prompt_state.setPromptText(PromptState::PROMPT_READY);
            }
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
        [&](Cursor& cursor, const std::vector<std::u16string_view> &args) -> std::optional<std::u16string> {
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
            const auto &edit_clear = cursor.clear();
            m_high_lighter.edit(edit_clear);

            const auto file_extension = std::filesystem::path(path).extension().string();
            m_high_lighter.setMode(file_extension);

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

            const auto &edit_insert = cursor.insert(all_line);
            m_high_lighter.edit(edit_insert);

            cursor.setName(path);
            cursor.setPosition(0, 0);
            m_editor_state.setFollowIndicator(true);
            return std::nullopt;
        },
        [&](const int32_t argumentIndex, const std::string_view input, const ItemCallback<char> &itemCallback) {
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
        [&](Cursor& cursor, const std::vector<std::u16string_view> &args) -> std::optional<std::u16string> {
            const auto cursor_name = std::filesystem::path(cursor.getName());
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
                m_command_manager.setCommandFeedback(
                    u"File already exists, overwrite ? [y/N]:",
                    std::u16string(u"save ").append(args[0]).append(u" -f"),
                    {u"n", u"y"},
                    [&](const std::u16string_view answer, const std::u16string_view command) -> std::optional<std::u16string> {
                        if (answer == u"y" || answer == u"Y") {
                            runCommand(command);
                            return std::nullopt;
                        }
                        return std::nullopt;
                    });
                return std::nullopt;
            }

            auto ofs = std::ofstream(file_to_save, std::ios::out);
            if(!ofs || !ofs.is_open()) {
                return std::u16string(u"Could not save ").append(args[0]).append(u".");
            }

            const auto line_count = cursor.getLineCount();
            for (auto line = 0; line < line_count; ++line) {
                const auto string = cursor.getString(line);
                ofs << utf8::utf16to8(string);
                if(line < line_count - 1) {
                    ofs << "\n";
                }
            }

            ofs.close();
            cursor.setName(file_to_save.string());
            return std::nullopt;
        },
        [&](const int32_t argumentIndex, const std::string_view input, const ItemCallback<char> &itemCallback) {
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
        [&](const Cursor& cursor, const std::vector<std::u16string_view> &args) -> std::optional<std::u16string> {
            (void) cursor;
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
        [&](const Cursor& cursor, const std::vector<std::u16string_view> &args) -> std::optional<std::u16string> {
            (void) cursor;
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
        [&](const Cursor& cursor, const std::vector<std::u16string_view> &args) -> std::optional<std::u16string> {
            (void) cursor;
            if (args.size() < 3 || (!args.empty() && args[1].empty())) {
                return u"Usage: bind <modifiers> <key> <command>";
            }

            const auto split_modifiers = CommandManager::split(args[0], u'+');

            auto modifier = 0;
            for (const auto &string_modifier : split_modifiers) {
                if (string_modifier == u"ctrl") {
                    modifier |= KMOD_CTRL;
                } else if (string_modifier == u"alt") {
                    modifier |= KMOD_ALT;
                } else if (string_modifier == u"shift") {
                    modifier |= KMOD_SHIFT;
                } else {
                    return std::u16string(u"Unknown modifier: ").append(string_modifier);
                }
            }

            if (modifier == 0) {
                return u"Expected at least one modifier.";
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
        [&](const Cursor& cursor, const std::vector<std::u16string_view> &args) -> std::optional<std::u16string> {
            (void) cursor;
            if (!args.empty()) {
                return u"Expected 0 argument.";
            }

            const auto font_size = m_theme.getFontSize();
            m_theme.setFontSize(font_size + 1);

            return std::nullopt;
        });

    m_command_manager.registerCommand("decrease_font_size",
        [&](const Cursor& cursor, const std::vector<std::u16string_view> &args) -> std::optional<std::u16string> {
            (void) cursor;
            if (!args.empty()) {
                return u"Expected 0 argument.";
            }

            const auto font_size = m_theme.getFontSize();
            m_theme.setFontSize(font_size - 1);

            return std::nullopt;
        });
}

void ApplicationWindow::registerActivatePromptCommand() {
    // Register activate_prompt command
    m_command_manager.registerCommand("activate_prompt",
        [&](const Cursor& cursor, const std::vector<std::u16string_view> &args) -> std::optional<std::u16string> {
            (void) cursor;
            if (!args.empty()) {
                return u"Expected 0 argument.";
            }

            // Set focus to prompt (since the editor had it if we run from a binding)
            m_focus_target = FocusTarget::Prompt;
            // Set prompt to running state
            m_prompt_state.setRunningState(PromptState::RunningState::Running);
            m_prompt_state.setPromptText(PromptState::PROMPT_ACTIVE);
            return std::nullopt;
        });
}
