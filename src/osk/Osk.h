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
#ifndef OSK_H
#define OSK_H

#include <SDL.h>

#include "../core/renderer/QuadProgram.h"
#include "../core/renderer/QuadBuffer.h"
#include "../core/theme/Theme.h"
#include "../core/View.h"
#include "OskState.h"


/**
 * @brief On-screen keyboard view, drawn as a bottom strip while visible.
 *
 * Injection, not integration: key taps push synthesized SDL_KEYDOWN/SDL_KEYUP events
 * (scancode + keycode + the sticky-modifier mask in keysym.mod) and SDL_TEXTINPUT for
 * printable keys through SDL_PushEvent, so the whole existing pipeline — bindings, chords,
 * prompt, editor text input — works unchanged and nothing downstream knows the OSK exists.
 * No SDL_TEXTINPUT is synthesized while Ctrl or Alt is latched, matching real keyboards.
 *
 * Two key pages share one 5-row geometry; the sticky Ctrl/Shift/Alt/AltGr keys latch on
 * tap (dot shown), hold on long-press, and every latched modifier releases after the next
 * key. Every held injecting key auto-repeats — like a physical keyboard — through the
 * InputRepeater in OskState, re-emitting under the sticky mask captured at press time.
 * The pad drives a key cursor via onPadInput once the OSK acquired the pad (lazily, on the
 * first pad press ControllerInput routes to it, and tracked by OskState); taps never move
 * the keyboard focus, so the OSK types into an active prompt as well as into the editor.
 */
class Osk final : public View<OskState> {
public:
    /**
     * @brief SDL user-event type carrying synthesized OSK text; data1 is an SDL_strdup'd UTF-8 string.
     *
     * sdl2-compat (SDL2 shimmed over SDL3) crashes on application-pushed SDL_TEXTINPUT
     * events, so the synthesized text rides this user event through the same queue instead;
     * the event pump turns it back into the normal text-input dispatch, preserving the
     * event order and keeping everything downstream unaware of the OSK.
     */
    [[nodiscard]] static uint32_t textEventType();

private:
    /** Milliseconds a sticky key must stay pressed to hold instead of latching. */
    static constexpr uint64_t LONG_PRESS_MS = 400;

    /**
     * @brief Draws the strip background and every key of the visible page.
     *
     * @param quadBuffer A reference to the quad buffer receiving the quads.
     * @param viewState A reference to the OSK view state (its pad focus shows the key cursor).
     */
    void drawKeys(QuadBuffer &quadBuffer, const OskState &viewState);

public:
    /**
     * @brief Constructs an Osk view instance.
     *
     * @param commandController Reference to the CommandManager instance.
     * @param theme Reference to the Theme manager for styling.
     * @param quadProgram Reference to the QuadProgram for rendering.
     */
    explicit Osk(GlobalRegistry<CursorContext> &commandController, Theme &theme, QuadProgram &quadProgram);

    /**
     * @brief Renders the on-screen keyboard; does nothing while hidden.
     *
     * @param context Reference to the cursor context.
     * @param viewState The associated OskState for layout/interaction data.
     * @param quadBuffer Reference to the quad buffer used to build this frame's geometry.
     * @param dt Time delta since the last frame.
     */
    void render(CursorContext &context, OskState &viewState, QuadBuffer &quadBuffer, float dt) override;

    /**
     * @brief Key events are never consumed: the OSK synthesizes them, it does not read them.
     *
     * @param context Reference to the cursor context.
     * @param viewState The OSK view state.
     * @param keyCode SDL key code.
     * @param keyModifier Key modifier mask.
     * @return Always false.
     */
    bool onKeyDown(CursorContext &context, OskState &viewState, SDL_Keycode keyCode, uint16_t keyModifier) const override;

    /**
     * @brief Text input is ignored: the OSK synthesizes it, it does not read it.
     *
     * @param context Reference to the cursor context.
     * @param viewState The OSK view state.
     * @param text UTF-8 encoded character input.
     */
    void onTextInput(CursorContext &context, OskState &viewState, const char *text) const override;

    /**
     * @brief Handles a press inside the OSK: taps the key under the point.
     *
     * Regular keys inject immediately and arm the hold auto-repeat when repeatable; sticky
     * keys wait for the release to tell a tap (latch) from a long-press (hold). The input
     * focus is never moved.
     *
     * @param context Reference to the cursor context.
     * @param viewState The OSK view state.
     * @param x Window-relative x coordinate of the press, in pixels.
     * @param y Window-relative y coordinate of the press, in pixels.
     */
    void onMouseDown(CursorContext &context, OskState &viewState, int32_t x, int32_t y) override;

    /**
     * @brief Handles the release ending a press: settles sticky keys and disarms the repeat.
     *
     * @param context Reference to the cursor context.
     * @param viewState The OSK view state.
     * @param x Window-relative x coordinate of the release, in pixels.
     * @param y Window-relative y coordinate of the release, in pixels.
     */
    void onMouseUp(CursorContext &context, OskState &viewState, int32_t x, int32_t y) override;

    /**
     * @brief Handles a pad input routed to the OSK while it has the pad focus.
     *
     * D-pad directions move the key cursor, A presses the key under it, B hands the pad back
     * to the bindings — or, over an active prompt, cancels it and keeps the pad, so one press
     * reads as one "get me out of here". Any other pad input is not handled and falls back to
     * the bindings.
     *
     * @param context Reference to the cursor context.
     * @param viewState The OSK view state.
     * @param padKeycode The pad pseudo-keycode (PadInput encoding) of the pressed input.
     * @return true when the input was handled by the OSK.
     */
    [[nodiscard]] bool onPadInput(CursorContext &context, OskState &viewState, SDL_Keycode padKeycode) const;

    /**
     * @brief Fires the held-key auto-repeat once its deadline passed, then re-arms it.
     *
     * Meant to run after the event poll loop, like the controller repeat tick.
     *
     * @param context Reference to the cursor context.
     * @param viewState The OSK view state.
     */
    void tickRepeat(CursorContext &context, OskState &viewState) const;
};


#endif //OSK_H
