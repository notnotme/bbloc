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
#ifndef INFO_BAR_H
#define INFO_BAR_H


#include "../core/base/GlobalRegistry.h"
#include "../core/renderer/QuadProgram.h"
#include "../core/renderer/QuadBuffer.h"
#include "../core/theme/Theme.h"
#include "../core/View.h"
#include "../core/ViewState.h"
#include "../core/CursorContext.h"


/**
 * @brief The InfoBar view, displayed at the top of the screen.
 *
 * This view renders contextual information like:
 * - Current file name
 * - Cursor position (line and column)
 * - Current highlight mode
 * - ect.
 */
class InfoBar final : public View<> {
private:
    /**
     * @brief: Draw the background layer of the info bar.
     *
     * @param viewState A reference to the InfoBar view state.
     */
    void drawBackground(const ViewState &viewState) const;

    /**
      * @brief: Draw the text layer of the info bar.
      *
      * @param context A reference to the cursor context.
      * @param viewState A reference to the InfoBar view state.
      */
    void drawText(const CursorContext &context, const ViewState &viewState) const;

public:
    /**
     * @brief Constructs the InfoBar view.
     *
     * @param commandController Reference to the command controller.
     * @param theme Reference to the Theme (fonts, colors, etc.).
     * @param quadProgram Reference to the quad shader program.
     * @param quadBuffer Reference to the quad buffer.
     */
    explicit InfoBar(GlobalRegistry<CursorContext> &commandController, Theme &theme, QuadProgram &quadProgram, QuadBuffer &quadBuffer);

    /**
     * @brief Renders the InfoBar.
     *
     * @param context Reference to the cursor context.
     * @param viewState The InfoBarState of this view.
     * @param dt Time delta since the last frame.
     */
    void render(CursorContext &context, ViewState &viewState, float dt) override;

    /**
     * @brief InfoBar does not handle key input.
     *
     * @return Always returns false.
     */
    bool onKeyDown(CursorContext &context, ViewState &viewState, SDL_Keycode keyCode, uint16_t keyModifier) const override;

    /** @brief InfoBar does not handle text input. */
    void onTextInput(CursorContext &context, ViewState &viewState, const char* text) const override;
};


#endif //INFO_BAR_H
