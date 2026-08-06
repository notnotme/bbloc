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
#ifndef POINTER_INPUT_H
#define POINTER_INPUT_H

#include <SDL.h>

#include "../core/CursorContextManager.h"
#include "../core/theme/Theme.h"
#include "../core/ViewState.h"
#include "../editor/Editor.h"
#include "../infobar/InfoBar.h"
#include "../osk/Osk.h"
#include "../osk/OskState.h"
#include "../prompt/Prompt.h"
#include "../prompt/PromptState.h"

/**
 * @brief Handles SDL pointer events: mouse buttons, motion, wheel, and touch fingers.
 *
 * The view under a left-button press captures the pointer: motion and release events keep
 * being routed to it until the button is released. Touch input replays the same capture:
 * one finger acts as the left button, two fingers scroll the active context.
 */
class PointerInput final {
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
        Prompt,   ///< The prompt received the press.
        Osk       ///< The on-screen keyboard received the press.
    };

    /**
     * @brief Touch gesture currently in progress.
     *
     * One finger acts as the left mouse button, routed through the MouseTarget capture;
     * two fingers scroll the active context. Scroll mode ends only when every finger has
     * lifted, so a trailing finger cannot start an accidental selection.
     */
    enum class TouchMode : uint8_t {
        None,    ///< No finger on the screen.
        Drag,    ///< Single finger: emulates a left-button press and drag.
        Scroll   ///< Two or more fingers: scrolls the active context.
    };

    /** Open cursor contexts; events are dispatched against the active one. */
    CursorContextManager &m_context_manager;

    /** Theme queried for the wheel scroll amounts (line height, font advance). */
    Theme &m_theme;

    /** Top info bar view. */
    InfoBar &m_info_bar;

    /** State tracking the info bar. */
    ViewState &m_info_bar_state;

    /** Main editor view. */
    Editor &m_editor;

    /** State object tracking the editor. */
    ViewState &m_editor_state;

    /** Bottom command prompt view. */
    Prompt &m_prompt;

    /** State object tracking the prompt. */
    PromptState &m_prompt_state;

    /** On-screen keyboard view. */
    Osk &m_osk;

    /** State object tracking the on-screen keyboard. */
    OskState &m_osk_state;

    /** View that received the current left-button press, None outside a press. */
    MouseTarget m_mouse_target;

    /** Gesture the fingers currently perform, None while the screen is untouched. */
    TouchMode m_touch_mode;

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
     * @brief Routes a press to the view whose rectangle contains it and captures that view.
     *
     * @param x Window-relative x coordinate of the press, in pixels.
     * @param y Window-relative y coordinate of the press, in pixels.
     */
    void press(int32_t x, int32_t y);

    /**
     * @brief Routes a drag motion to the captured view.
     *
     * @param x Window-relative x coordinate of the pointer, in pixels.
     * @param y Window-relative y coordinate of the pointer, in pixels.
     */
    void drag(int32_t x, int32_t y);

    /**
     * @brief Lets the captured view end its drag, then releases the capture.
     *
     * @param x Window-relative x coordinate of the release, in pixels.
     * @param y Window-relative y coordinate of the release, in pixels.
     */
    void release(int32_t x, int32_t y);

public:
    /** @brief Deleted copy constructor. */
    PointerInput(const PointerInput &) = delete;

    /** @brief Deleted copy assignment operator. */
    PointerInput &operator=(const PointerInput &) = delete;

    /**
     * @brief Constructs the handler with references to the objects it dispatches to.
     *
     * @param contextManager Manager providing the active cursor context.
     * @param theme Theme queried for the wheel scroll amounts.
     * @param infoBar The info bar view.
     * @param infoBarState State of the info bar view.
     * @param editor The editor view.
     * @param editorState State of the editor view.
     * @param prompt The prompt view.
     * @param promptState State of the prompt view.
     * @param osk The on-screen keyboard view.
     * @param oskState State of the on-screen keyboard view.
     */
    explicit PointerInput(CursorContextManager &contextManager, Theme &theme, InfoBar &infoBar, ViewState &infoBarState, Editor &editor, ViewState &editorState, Prompt &prompt, PromptState &promptState, Osk &osk, OskState &oskState);

    /**
     * @brief Handles an SDL_MOUSEBUTTONDOWN event; left button only.
     *
     * @param event The mouse button event.
     */
    void onMouseDown(const SDL_MouseButtonEvent &event);

    /**
     * @brief Handles an SDL_MOUSEMOTION event; only matters during a left-button drag.
     *
     * @param event The mouse motion event.
     */
    void onMouseMotion(const SDL_MouseMotionEvent &event);

    /**
     * @brief Handles an SDL_MOUSEBUTTONUP event; releases the capture.
     *
     * @param event The mouse button event.
     */
    void onMouseUp(const SDL_MouseButtonEvent &event);

    /**
     * @brief Handles an SDL_MOUSEWHEEL event: scrolls the active context.
     *
     * @param event The mouse wheel event.
     */
    void onMouseWheel(const SDL_MouseWheelEvent &event);

    /**
     * @brief Handles an SDL_FINGERDOWN event.
     *
     * A first finger starts a drag routed like a mouse press; a second finger ends the
     * drag and turns the gesture into a scroll.
     *
     * @param event The touch finger event.
     * @param windowWidth Current window width in pixels, used to scale normalized coordinates.
     * @param windowHeight Current window height in pixels, used to scale normalized coordinates.
     */
    void onFingerDown(const SDL_TouchFingerEvent &event, int32_t windowWidth, int32_t windowHeight);

    /**
     * @brief Handles an SDL_FINGERMOTION event: drags the captured view or scrolls the context.
     *
     * @param event The touch finger event.
     * @param windowWidth Current window width in pixels, used to scale normalized coordinates.
     * @param windowHeight Current window height in pixels, used to scale normalized coordinates.
     */
    void onFingerMotion(const SDL_TouchFingerEvent &event, int32_t windowWidth, int32_t windowHeight);

    /**
     * @brief Handles an SDL_FINGERUP event: ends the drag, or the scroll once every finger lifted.
     *
     * @param event The touch finger event.
     * @param windowWidth Current window width in pixels, used to scale normalized coordinates.
     * @param windowHeight Current window height in pixels, used to scale normalized coordinates.
     */
    void onFingerUp(const SDL_TouchFingerEvent &event, int32_t windowWidth, int32_t windowHeight);
};


#endif //POINTER_INPUT_H
