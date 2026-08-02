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
void View<TState>::resizeWindow(const int32_t width, const int32_t height) {
    m_window_width = width;
    m_window_height = height;
}

template <typename TState>
void View<TState>::drawQuad(QuadBuffer &quadBuffer, const int32_t x, const int32_t y, const int32_t width, const int32_t height, const Color &color) const {
    quadBuffer.insert(
        static_cast<int16_t>(x),
        static_cast<int16_t>(y),
        static_cast<uint16_t>(width),
        static_cast<uint16_t>(height),
        color.red, color.green, color.blue, color.alpha);
}

template<typename TState>
void View<TState>::drawCharacter(QuadBuffer &quadBuffer, const int32_t x, const int32_t y, const AtlasEntry &character, const Color &color) const {
    quadBuffer.insert(
        static_cast<int16_t>(x + character.bearing_x),
        static_cast<int16_t>(y - character.bearing_y),
        character.width, character.height,
        character.texture_s, character.texture_t, character.layer,
        color.red, color.green, color.blue, color.alpha);

}

#endif //VIEW_H
