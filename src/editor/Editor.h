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
#ifndef EDITOR_H
#define EDITOR_H

#include <SDL.h>

#include "../core/base/GlobalRegistry.h"
#include "../core/cvar/CVarBool.h"
#include "../core/renderer/QuadProgram.h"
#include "../core/renderer/QuadBuffer.h"
#include "../core/theme/Theme.h"
#include "../core/View.h"
#include "../core/ViewState.h"
#include "../core/CursorContext.h"


/**
 * @brief Main text editor view responsible for rendering text and handling input.
 *
 * The Editor view manages the rendering the cursor buffer, processing user input,
 * tracking scroll state, everything via CursorContext.
 */
class Editor final : public View<> {
private:
    /** CVar for toggling tab-to-space replacement in input. */
    std::shared_ptr<CVarBool> m_is_tab_to_space;

private:
    /** @brief Registers the tab_to_space cvar into the command manager. */
    void registerTabToSpaceCVar() const;

    /**
     * @brief: Compute scroll position and max scroll for the horizontal and vertical axis.
     *
     * @param context A reference to the cursor context.
     * @param viewState A reference to the Editor view state.
     * @param marginWidth The width of the margin, without the border size.
     */
    void updateScroll(CursorContext &context, const ViewState &viewState, int32_t marginWidth) const;

    /**
     * @brief: Draw the background layer of the editor.
     *
     * @param viewState A reference to the Editor view state.
     * @param marginWidth The width of the margin, without the border size.
     */
    void drawBackground(const ViewState &viewState, int32_t marginWidth) const;

    /**
     * @brief: Draw the text layer in the left margin of the editor.
     *
     * @param context A reference to the cursor context.
     * @param viewState A reference to the Editor view state.
     * @param lineCountWidth The width in pixel of the greatest line number.
     * @param scrollY The editor y scroll offset.
     */
    void drawMarginText(const CursorContext &context, const ViewState &viewState, int32_t lineCountWidth, int32_t scrollY) const;

    /**
     * @brief Draw the text layer (glyphs, selection, and cursor indicator) of the editor.
     *
     * @param context A reference to the cursor context.
     * @param viewState A reference to the Editor view state.
     * @param scrollX The editor x scroll offset.
     * @param scrollY The editor y scroll offset.
     * @param marginWidth The width of the margin, without the border size.
     */
    void drawText(const CursorContext &context, const ViewState &viewState, int32_t scrollX, int32_t scrollY, int32_t marginWidth) const;

public:
    /**
     * @brief Constructs the Editor view.
     *
     * @param commandController Reference to the CommandController.
     * @param theme Reference to the Theme for rendering.
     * @param quadProgram Reference to the QuadProgram shader.
     * @param quadBuffer Reference to the geometry buffer.
     */
    explicit Editor(GlobalRegistry<CursorContext> &commandController, Theme &theme, QuadProgram &quadProgram, QuadBuffer &quadBuffer);

    /**
     * @brief Renders the text editor to the screen.
     *
     * @param context Reference to the cursor context.
     * @param viewState State of the editor view.
     * @param dt Time delta since the last frame.
     */
    void render(CursorContext &context, ViewState &viewState, float dt) override;

    /**
     * @brief Handles key down events in the editor.
     *
     * @param context Reference to the cursor context.
     * @param viewState State of the editor view.
     * @param keyCode SDL key code.
     * @param keyModifier Modifier bitmask (Shift, Ctrl, etc).
     * @return True if the event was handled.
     */
    bool onKeyDown(CursorContext &context, ViewState &viewState, SDL_Keycode keyCode, uint16_t keyModifier) const override;

    /**
     * @brief Handles text input events in the editor.
     *
     * @param context Reference to the cursor context.
     * @param viewState State of the editor view.
     * @param text UTF-8 encoded character input from SDL_TEXTINPUT.
     */
    void onTextInput(CursorContext &context, ViewState &viewState, const char* text) const override;
};


#endif //EDITOR_H
