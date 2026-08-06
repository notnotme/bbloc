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
#include "PointerInput.h"

#include <algorithm>
#include <cmath>


PointerInput::PointerInput(CursorContextManager &contextManager, Theme &theme, InfoBar &infoBar, ViewState &infoBarState, Editor &editor, ViewState &editorState, Prompt &prompt, PromptState &promptState, Osk &osk, OskState &oskState)
    : m_context_manager(contextManager),
      m_theme(theme),
      m_info_bar(infoBar),
      m_info_bar_state(infoBarState),
      m_editor(editor),
      m_editor_state(editorState),
      m_prompt(prompt),
      m_prompt_state(promptState),
      m_osk(osk),
      m_osk_state(oskState),
      m_mouse_target(MouseTarget::None),
      m_touch_mode(TouchMode::None) {}

bool PointerInput::viewContains(const ViewState &viewState, const int32_t x, const int32_t y) {
    const auto position_x = viewState.getPositionX();
    const auto position_y = viewState.getPositionY();
    return x >= position_x && x < position_x + viewState.getWidth()
        && y >= position_y && y < position_y + viewState.getHeight();
}

void PointerInput::press(const int32_t x, const int32_t y) {
    // Route the press to the view whose rectangle contains it, and capture that
    // view: motion and release keep going to it until the press ends
    auto &context = m_context_manager.active();
    if (m_osk_state.isVisible() && viewContains(m_osk_state, x, y)) {
        // The on-screen keyboard consumes its taps; they never move the input focus
        m_mouse_target = MouseTarget::Osk;
        m_osk.onMouseDown(context, m_osk_state, x, y);
    } else if (viewContains(m_editor_state, x, y)) {
        m_mouse_target = MouseTarget::Editor;
        m_editor.onMouseDown(context, m_editor_state, x, y);
    } else if (viewContains(m_prompt_state, x, y)) {
        m_mouse_target = MouseTarget::Prompt;
        m_prompt.onMouseDown(context, m_prompt_state, x, y);
    } else if (viewContains(m_info_bar_state, x, y)) {
        m_mouse_target = MouseTarget::InfoBar;
        m_info_bar.onMouseDown(context, m_info_bar_state, x, y);
    }
}

void PointerInput::drag(const int32_t x, const int32_t y) {
    // Keep routing the motion to the view that received the press, even when
    // the pointer leaves its rectangle
    auto &context = m_context_manager.active();
    switch (m_mouse_target) {
        case MouseTarget::Editor:
            m_editor.onMouseMotion(context, m_editor_state, x, y);
        break;
        case MouseTarget::Prompt:
            m_prompt.onMouseMotion(context, m_prompt_state, x, y);
        break;
        case MouseTarget::InfoBar:
            m_info_bar.onMouseMotion(context, m_info_bar_state, x, y);
        break;
        case MouseTarget::Osk:
            m_osk.onMouseMotion(context, m_osk_state, x, y);
        break;
        default:
        break;
    }
}

void PointerInput::release(const int32_t x, const int32_t y) {
    // Release the capture after letting the pressed view end its drag
    auto &context = m_context_manager.active();
    switch (m_mouse_target) {
        case MouseTarget::Editor:
            m_editor.onMouseUp(context, m_editor_state, x, y);
        break;
        case MouseTarget::Prompt:
            m_prompt.onMouseUp(context, m_prompt_state, x, y);
        break;
        case MouseTarget::InfoBar:
            m_info_bar.onMouseUp(context, m_info_bar_state, x, y);
        break;
        case MouseTarget::Osk:
            m_osk.onMouseUp(context, m_osk_state, x, y);
        break;
        default:
        break;
    }
    m_mouse_target = MouseTarget::None;
}

void PointerInput::onMouseDown(const SDL_MouseButtonEvent &event) {
    // Left button only; other buttons are ignored for now
    if (event.button != SDL_BUTTON_LEFT) {
        return;
    }

    press(event.x, event.y);
}

void PointerInput::onMouseMotion(const SDL_MouseMotionEvent &event) {
    // Motion only matters during a left-button drag
    if (m_mouse_target == MouseTarget::None || !(event.state & SDL_BUTTON_LMASK)) {
        return;
    }

    drag(event.x, event.y);
}

void PointerInput::onMouseUp(const SDL_MouseButtonEvent &event) {
    if (event.button != SDL_BUTTON_LEFT || m_mouse_target == MouseTarget::None) {
        return;
    }

    release(event.x, event.y);
}

void PointerInput::onMouseWheel(const SDL_MouseWheelEvent &event) {
    // We must have an updated value for the line_height, so request the size from the theme now
    auto &context = m_context_manager.active();
    const auto line_height = m_theme.getLineHeight();
    const auto scroll_amount = event.y * -line_height;
    context.scroll.y = context.scroll.y + scroll_amount;
    // Horizontal wheel: positive wheel.x means scrolling right, matching a scroll.x increase
    context.scroll.x = context.scroll.x + event.x * m_theme.getFontAdvance();
    context.wants_redraw = true;
}

void PointerInput::onFingerDown(const SDL_TouchFingerEvent &event, const int32_t windowWidth, const int32_t windowHeight) {
    const auto x = static_cast<int32_t>(event.x * static_cast<float>(windowWidth));
    const auto y = static_cast<int32_t>(event.y * static_cast<float>(windowHeight));
    const auto finger_count = SDL_GetNumTouchFingers(event.touchId);

    if (m_touch_mode == TouchMode::None && finger_count == 1) {
        // First finger acts as a left-button press: route and capture like a mouse
        // press. Drag mode is entered even when no view contains the point,
        // so a second finger can still turn the gesture into a scroll
        m_touch_mode = TouchMode::Drag;
        press(x, y);
    } else if (finger_count >= 2 && m_touch_mode != TouchMode::Scroll) {
        // A second finger turns the gesture into a scroll: end the drag first so
        // the captured view closes its state (coordinates are ignored on release)
        if (m_mouse_target != MouseTarget::None) {
            release(x, y);
        }
        m_touch_mode = TouchMode::Scroll;
    }
}

void PointerInput::onFingerMotion(const SDL_TouchFingerEvent &event, const int32_t windowWidth, const int32_t windowHeight) {
    if (m_touch_mode == TouchMode::Scroll) {
        // Every finger reports its own motion, so split each event by the live
        // finger count to track the fingers 1:1. Fingers moving down reveal
        // earlier lines (scroll.y decreases), matching the wheel direction above.
        // follow_indicator is left untouched so the render-time clamp applies
        const auto finger_count = std::max(1, SDL_GetNumTouchFingers(event.touchId));
        auto &context = m_context_manager.active();
        context.scroll.x = context.scroll.x - std::lround(event.dx * static_cast<float>(windowWidth) / static_cast<float>(finger_count));
        context.scroll.y = context.scroll.y - std::lround(event.dy * static_cast<float>(windowHeight) / static_cast<float>(finger_count));
        context.wants_redraw = true;
    } else if (m_touch_mode == TouchMode::Drag && m_mouse_target != MouseTarget::None) {
        // Single-finger drag: same routing as a captured mouse motion
        const auto x = static_cast<int32_t>(event.x * static_cast<float>(windowWidth));
        const auto y = static_cast<int32_t>(event.y * static_cast<float>(windowHeight));
        drag(x, y);
    }
}

void PointerInput::onFingerUp(const SDL_TouchFingerEvent &event, const int32_t windowWidth, const int32_t windowHeight) {
    // SDL removes the finger before posting the event: 0 means the last one lifted
    const auto finger_count = SDL_GetNumTouchFingers(event.touchId);

    if (m_touch_mode == TouchMode::Drag) {
        // Release the capture after letting the pressed view end its drag
        if (m_mouse_target != MouseTarget::None) {
            const auto x = static_cast<int32_t>(event.x * static_cast<float>(windowWidth));
            const auto y = static_cast<int32_t>(event.y * static_cast<float>(windowHeight));
            release(x, y);
        }
        m_touch_mode = TouchMode::None;
    }

    // Scroll mode ends only when every finger has lifted, so a trailing finger
    // keeps scrolling and can never start an accidental selection
    if (finger_count == 0) {
        m_touch_mode = TouchMode::None;
    }
}
