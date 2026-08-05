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
#ifndef VIEW_H
#define VIEW_H

#include <algorithm>
#include <limits>

#include <SDL.h>

#include "renderer/QuadProgram.h"
#include "renderer/QuadBuffer.h"
#include "theme/Theme.h"
#include "CursorContext.h"
#include "ViewState.h"


/**
 * @brief Base class for rendering a view in the application (e.g., editor, console).
 *
 * This class defines the interface and common rendering infrastructure shared by different views.
 * It is templated on a optional view state (`TState`, defaults to `ViewState`).
 *
 * Derived classes must implement specific behaviors like input handling and rendering.
 *
 * @tparam TState State type for additional view-specific information.
 */
template <typename TState = ViewState>
class View {
protected:
    /** Lowest quad position the 16-bit vertex format holds; staged quads saturate at it. */
    static constexpr int32_t MIN_QUAD_POSITION = std::numeric_limits<int16_t>::min();

    /** Highest quad position the 16-bit vertex format holds; staged quads saturate at it. */
    static constexpr int32_t MAX_QUAD_POSITION = std::numeric_limits<int16_t>::max();

    /** Largest quad size the 16-bit vertex format holds; staged quads saturate at it. */
    static constexpr int32_t MAX_QUAD_SIZE = std::numeric_limits<uint16_t>::max();

    /** Reference to a command controller. */
    GlobalRegistry<CursorContext> &m_command_controller;

    /** Reference to the theme used for rendering (colors, fonts, etc.). */
    Theme &m_theme;

    /** Reference to the shader program for quad rendering. */
    QuadProgram &m_quad_program;

    /** Current window width in pixels. */
    int32_t m_window_width;

    /** Current window height in pixels. */
    int32_t m_window_height;

    /**
     * @brief Helper to push a new quad into the quad buffer.
     *
     * @param quadBuffer Reference to the quad buffer receiving the quad.
     * @param x The x position of the quad.
     * @param y The y position of the quad.
     * @param width The width of the quad.
     * @param height The height of the quad.
     * @param color Reference to the color to be used by this quad.
     */
    void drawQuad(QuadBuffer &quadBuffer, int32_t x, int32_t y, int32_t width, int32_t height, const Color &color) const;

    /**
     * @brief Helper to push a new character inside the quad buffer.
     *
     * @param quadBuffer Reference to the quad buffer receiving the quad.
     * @param x The x position of the character.
     * @param y The y position of the character.
     * @param character The character to draw (from AtlasEntry).
     * @param color The color to be used to draw this character.
     */
    void drawCharacter(QuadBuffer &quadBuffer, int32_t x, int32_t y, const AtlasEntry &character, const Color &color) const;

public:
    /** @brief Deleted copy constructor. */
    View(const View &) = delete;

    /** @brief Deleted copy assignment operator. */
    View &operator=(const View &) = delete;

    /** @brief For inheritance */
    virtual ~View() = default;

    /**
     * @brief Constructs a view with references to rendering and theme resources.
     *
     * @param commandController Reference to a command controller.
     * @param theme Reference to the theme manager.
     * @param quadProgram Reference to the quad shader program.
     */
    explicit View(GlobalRegistry<CursorContext> &commandController, Theme &theme, QuadProgram &quadProgram);

    /**
     * @brief Renders the view contents.
     *
     * @param context Reference to the cursor context to render.
     * @param viewState Reference to the view-specific state.
     * @param quadBuffer Reference to the quad buffer used to build this frame's geometry.
     * @param dt Delta time in seconds (useful for animations or transitions).
     */
    virtual void render(CursorContext &context, TState &viewState, QuadBuffer &quadBuffer, float dt) = 0;

    /**
     * @brief Handles key press events.
     *
     * @param context Reference to the cursor context to handle keys for.
     * @param viewState Reference to the view state.
     * @param keyCode The SDL key code.
     * @param keyModifier Bitmask of modifier keys (CTRL, ALT, etc.).
     * @return true if the key was handled, false otherwise.
     */
    virtual bool onKeyDown(CursorContext &context, TState &viewState, SDL_Keycode keyCode, uint16_t keyModifier) const = 0;

    /**
     * @brief Handles UTF-8 text input events (e.g., from typing).
     *
     * @param context Reference to the cursor context to handle text input for.
     * @param viewState Reference to the view state.
     * @param text UTF-8 encoded input text (from SDL_TEXTINPUT).
     */
    virtual void onTextInput(CursorContext &context, TState &viewState, const char* text) const = 0;

    /**
     * @brief Handles a left mouse button press inside the view.
     *
     * The default implementation ignores the event; views without mouse interactions keep it.
     *
     * @param context Reference to the cursor context to handle the press for.
     * @param viewState Reference to the view state.
     * @param x Window-relative x coordinate of the press, in pixels.
     * @param y Window-relative y coordinate of the press, in pixels.
     */
    virtual void onMouseDown(CursorContext &context, TState &viewState, int32_t x, int32_t y);

    /**
     * @brief Handles mouse motion while the left button is held after a press in this view.
     *
     * Motion events keep flowing to the view that received the press, even when the pointer
     * leaves its rectangle. The default implementation ignores the event.
     *
     * @param context Reference to the cursor context to handle the motion for.
     * @param viewState Reference to the view state.
     * @param x Window-relative x coordinate of the pointer, in pixels.
     * @param y Window-relative y coordinate of the pointer, in pixels.
     */
    virtual void onMouseMotion(CursorContext &context, TState &viewState, int32_t x, int32_t y);

    /**
     * @brief Handles the left mouse button release ending a press started in this view.
     *
     * The default implementation ignores the event.
     *
     * @param context Reference to the cursor context to handle the release for.
     * @param viewState Reference to the view state.
     * @param x Window-relative x coordinate of the release, in pixels.
     * @param y Window-relative y coordinate of the release, in pixels.
     */
    virtual void onMouseUp(CursorContext &context, TState &viewState, int32_t x, int32_t y);

    /**
     * @brief Updates the internal window size for the view (e.g., after a resize event).
     *
     * @param width New window width in pixels.
     * @param height New window height in pixels.
     */
    void resizeWindow(int32_t width, int32_t height);
};

template <typename TState>
View<TState>::View(GlobalRegistry<CursorContext> &commandController, Theme &theme, QuadProgram &quadProgram)
    : m_command_controller(commandController),
      m_theme(theme),
      m_quad_program(quadProgram),
      m_window_width(0),
      m_window_height(0) {}

template <typename TState>
void View<TState>::onMouseDown(CursorContext &context, TState &viewState, const int32_t x, const int32_t y) {
    // No-op by default
    (void) context;
    (void) viewState;
    (void) x;
    (void) y;
}

template <typename TState>
void View<TState>::onMouseMotion(CursorContext &context, TState &viewState, const int32_t x, const int32_t y) {
    // No-op by default
    (void) context;
    (void) viewState;
    (void) x;
    (void) y;
}

template <typename TState>
void View<TState>::onMouseUp(CursorContext &context, TState &viewState, const int32_t x, const int32_t y) {
    // No-op by default
    (void) context;
    (void) viewState;
    (void) x;
    (void) y;
}

template <typename TState>
void View<TState>::resizeWindow(const int32_t width, const int32_t height) {
    m_window_width = width;
    m_window_height = height;
}

template <typename TState>
void View<TState>::drawQuad(QuadBuffer &quadBuffer, const int32_t x, const int32_t y, const int32_t width, const int32_t height, const Color &color) const {
    // Plain quads are not culled against the viewport before being staged, unlike glyphs: a far
    // horizontal scroll pushes them past what the vertex format holds. Clamping saturates them at
    // the edge instead of letting the narrowing wrap them around to the opposite side.
    quadBuffer.insert(
        static_cast<int16_t>(std::clamp(x, MIN_QUAD_POSITION, MAX_QUAD_POSITION)),
        static_cast<int16_t>(std::clamp(y, MIN_QUAD_POSITION, MAX_QUAD_POSITION)),
        static_cast<uint16_t>(std::clamp(width, 0, MAX_QUAD_SIZE)),
        static_cast<uint16_t>(std::clamp(height, 0, MAX_QUAD_SIZE)),
        color.red, color.green, color.blue, color.alpha);
}

template<typename TState>
void View<TState>::drawCharacter(QuadBuffer &quadBuffer, const int32_t x, const int32_t y, const AtlasEntry &character, const Color &color) const {
    // Same saturation as drawQuad: a position past what the vertex format holds is clamped to the
    // edge instead of being wrapped around to the opposite side by the narrowing.
    quadBuffer.insert(
        static_cast<int16_t>(std::clamp(x + character.bearing_x, MIN_QUAD_POSITION, MAX_QUAD_POSITION)),
        static_cast<int16_t>(std::clamp(y - character.bearing_y, MIN_QUAD_POSITION, MAX_QUAD_POSITION)),
        character.width, character.height,
        character.texture_s, character.texture_t, character.layer,
        color.red, color.green, color.blue, color.alpha);

}

#endif //VIEW_H
