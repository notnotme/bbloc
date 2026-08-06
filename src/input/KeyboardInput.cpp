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
#include "KeyboardInput.h"

#include "../core/FocusTarget.h"


KeyboardInput::KeyboardInput(CommandRunner &commandRunner, CursorContextManager &contextManager, Editor &editor, ViewState &editorState, Prompt &prompt, PromptState &promptState)
    : m_command_runner(commandRunner),
      m_context_manager(contextManager),
      m_editor(editor),
      m_editor_state(editorState),
      m_prompt(prompt),
      m_prompt_state(promptState) {}

void KeyboardInput::onKeyDown(const SDL_KeyboardEvent &event) {
    // Chords are shortcuts, never editing keys: skip the focused view and go straight
    // to the bindings (same rationale as the onTextInput chord rule below)
    const auto is_chord = event.keysym.mod & (KMOD_CTRL | KMOD_LALT);

    if (!is_chord) {
        // The prompt dispatches a command on Return, which can switch the active context
        // or close (and destroy) this one: re-read active() before touching it afterwards.
        auto &context = m_context_manager.active();
        switch (context.focus_target) {
            // The Osk focus only redirects the pad; the physical keyboard keeps editing the buffer.
            case FocusTarget::Osk:
            case FocusTarget::Editor:
                if (m_editor.onKeyDown(context, m_editor_state, event.keysym.sym, event.keysym.mod)) {
                    // If the view return true, the text changed: redraw the views
                    context.search.resetMatches();
                    context.wants_redraw = true;
                    return;
                }
            break;
            case FocusTarget::Prompt:
                if (m_prompt.onKeyDown(context, m_prompt_state, event.keysym.sym, event.keysym.mod)) {
                    // If the view return true, then redraw the views.
                    // `context` may be gone by now (the prompt ran "buffer close"): flag the new active one.
                    m_context_manager.active().wants_redraw = true;
                    return;
                }
            break;
        }
    }

    // Not consumed by the focused view: fall back to the key bindings
    m_command_runner.runBoundCommand(event.keysym.sym, event.keysym.mod);
}

void KeyboardInput::onTextInput(const SDL_TextInputEvent &event) {
    // Don't type text for shortcut chords: X11 still delivers TEXTINPUT for Ctrl/Alt+letter
    const auto block_text_input = SDL_GetModState() & (KMOD_CTRL | KMOD_LALT);
    if (block_text_input) {
        return;
    }

    // Redirect to input focus. We always redraw new characters.
    auto &context = m_context_manager.active();
    context.wants_redraw = true;
    switch (context.focus_target) {
        // The Osk focus only redirects the pad; typed text still goes to the editor.
        case FocusTarget::Osk:
        case FocusTarget::Editor:
            m_editor.onTextInput(context, m_editor_state, event.text);
            context.search.resetMatches();
            break;
        case FocusTarget::Prompt:
            m_prompt.onTextInput(context, m_prompt_state, event.text);
            break;
    }
}
