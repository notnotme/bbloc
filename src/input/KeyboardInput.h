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
#ifndef KEYBOARD_INPUT_H
#define KEYBOARD_INPUT_H

#include <SDL.h>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/base/CommandRunner.h"
#include "../core/CursorContextManager.h"
#include "../core/ViewState.h"
#include "../editor/Editor.h"
#include "../prompt/Prompt.h"
#include "../prompt/PromptState.h"

/**
 * @brief Handles SDL keyboard events: key presses and text input.
 *
 * Key presses go to the focused view first (unless the modifiers form a shortcut chord),
 * then fall back to the key bindings, run through CommandRunner::runBoundCommand.
 * Text input is routed to the focused view, with chords blocked.
 */
class KeyboardInput final {
private:
    /** The command runner owning the timed bound-command execution. */
    CommandRunner &m_command_runner;

    /** Open cursor contexts; events are dispatched against the active one. */
    CursorContextManager &m_context_manager;

    /** Main editor view. */
    Editor &m_editor;

    /** State object tracking the editor. */
    ViewState &m_editor_state;

    /** Bottom command prompt view. */
    Prompt &m_prompt;

    /** State object tracking the prompt. */
    PromptState &m_prompt_state;

public:
    /** @brief Deleted copy constructor. */
    KeyboardInput(const KeyboardInput &) = delete;

    /** @brief Deleted copy assignment operator. */
    KeyboardInput &operator=(const KeyboardInput &) = delete;

    /**
     * @brief Constructs the handler with references to the objects it dispatches to.
     *
     * @param commandRunner The command runner, used to run key-bound commands.
     * @param contextManager Manager providing the active cursor context.
     * @param editor The editor view.
     * @param editorState State of the editor view.
     * @param prompt The prompt view.
     * @param promptState State of the prompt view.
     */
    explicit KeyboardInput(CommandRunner &commandRunner, CursorContextManager &contextManager, Editor &editor, ViewState &editorState, Prompt &prompt, PromptState &promptState);

    /**
     * @brief Handles an SDL_KEYDOWN event.
     *
     * Chords (Ctrl/Alt held) skip the focused view and go straight to the bindings;
     * otherwise the focused view gets the key first and the bindings are the fallback.
     *
     * @param event The keyboard event.
     */
    void onKeyDown(const SDL_KeyboardEvent &event);

    /**
     * @brief Handles an SDL_TEXTINPUT event.
     *
     * Routes the typed text to the focused view, unless a shortcut chord is held
     * (X11 still delivers TEXTINPUT for Ctrl/Alt+letter).
     *
     * @param event The text input event.
     */
    void onTextInput(const SDL_TextInputEvent &event);
};


#endif //KEYBOARD_INPUT_H
