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
#ifndef CONTROLLER_INPUT_H
#define CONTROLLER_INPUT_H

#include <array>
#include <vector>

#include <SDL.h>

#include "../core/base/AutoCompleteCallback.h"
#include "../core/base/CommandRunner.h"
#include "../core/base/PadInput.h"
#include "../core/CursorContextManager.h"
#include "../osk/Osk.h"
#include "../osk/OskState.h"
#include "InputRepeater.h"

/**
 * @brief Handles SDL game-controller events: hotplug, buttons, and axis motions.
 *
 * Buttons and axis directions are encoded as pad pseudo-keycodes (PadInput) and dispatched
 * through CommandRunner::runBoundCommand, exactly like keyboard shortcuts. The two shoulders
 * are not dispatched: they build the live L/R modifier mask instead. Sticks and triggers act
 * as digital pseudo-buttons, with hysteresis between the press and release thresholds so a
 * half-deflected stick cannot flutter. A successfully dispatched press arms an auto-repeat
 * (delay then fast interval) that the event loop ticks between waits.
 *
 * While the on-screen keyboard is visible, the first d-pad/A press acquires the pad focus
 * for it (lazily — mouse and touch users never see its key cursor), and from then on the
 * d-pad and the A/B buttons drive the key cursor instead of running bindings — the same
 * "focused view first, bindings as fallback" shape KeyboardInput has. Every other pad
 * input stays on the bindings.
 *
 * Every connected pad feeds the same state, like a keyboard: events are not filtered by
 * controller instance.
 */
class ControllerInput final {
private:
    /** Axis magnitude (of 32767) at or above which a direction counts as pressed. */
    static constexpr int32_t AXIS_PRESS_THRESHOLD = 16000;

    /** Axis magnitude (of 32767) below which a pressed direction releases; the gap absorbs jitter. */
    static constexpr int32_t AXIS_RELEASE_THRESHOLD = 11000;

    /** The command runner owning the timed bound-command execution. */
    CommandRunner &m_command_runner;

    /** Open cursor contexts; the pad focus is read from the active one. */
    CursorContextManager &m_context_manager;

    /** On-screen keyboard view, receiving the pad inputs while it has the focus. */
    Osk &m_osk;

    /** State object tracking the on-screen keyboard. */
    OskState &m_osk_state;

    /** Open controller handles, one per connected pad; hotplug adds and removes them. */
    std::vector<SDL_GameController *> m_controllers;

    /** Live pad modifier mask built from the held shoulders (KMOD_PAD_L / KMOD_PAD_R). */
    uint16_t m_pad_modifiers;

    /** Auto-repeat state of the held pad input (delay then fast interval). */
    InputRepeater m_repeater;

    /** Pressed flag per (axis, direction); index 1 holds the positive direction. */
    std::array<std::array<bool, 2>, SDL_CONTROLLER_AXIS_MAX> m_axis_pressed;

    /**
     * @brief Routes a pad input: the focused on-screen keyboard first, the bindings as fallback.
     *
     * @param keycode The pad pseudo-keycode being dispatched.
     * @param modifiers The pad modifier mask to dispatch with.
     * @return true when the input should auto-repeat while held: a bound command ran, or the
     *         on-screen keyboard consumed a d-pad direction (its key cursor repeats; A/B do not).
     */
    bool dispatch(SDL_Keycode keycode, uint16_t modifiers);

    /**
     * @brief Dispatches a pad pseudo-button press and arms the auto-repeat on success.
     *
     * A new press always disarms the previous repeat first; the repeat only arms when a
     * bound command actually ran, capturing the modifiers held at press time.
     *
     * @param keycode The pad pseudo-keycode being pressed.
     */
    void press(SDL_Keycode keycode);

    /**
     * @brief Handles a pad pseudo-button release: disarms the repeat when it is the held input.
     *
     * @param keycode The pad pseudo-keycode being released.
     */
    void release(SDL_Keycode keycode);

    /**
     * @brief Applies the press/release hysteresis to one direction of an axis.
     *
     * A press transition behaves like a button press of the direction pseudo-button, a
     * release transition like its button release.
     *
     * @param axis The SDL controller axis being evaluated.
     * @param positive True for the positive direction of the axis, false for the negative one.
     * @param magnitude The axis deflection along that direction, positive when deflected into it.
     */
    void updateAxisDirection(SDL_GameControllerAxis axis, bool positive, int32_t magnitude);

public:
    /** @brief Deleted copy constructor. */
    ControllerInput(const ControllerInput &) = delete;

    /** @brief Deleted copy assignment operator. */
    ControllerInput &operator=(const ControllerInput &) = delete;

    /**
     * @brief Constructs the handler with references to the objects it dispatches to.
     *
     * @param commandRunner The command runner, used to run pad-bound commands.
     * @param contextManager Manager providing the active cursor context (pad focus).
     * @param osk The on-screen keyboard view.
     * @param oskState State of the on-screen keyboard view.
     */
    explicit ControllerInput(CommandRunner &commandRunner, CursorContextManager &contextManager, Osk &osk, OskState &oskState);

    /**
     * @brief Handles an SDL_CONTROLLERDEVICEADDED event: opens and stores the pad.
     *
     * SDL also posts this event for pads already connected at init, so hotplug covers the
     * startup case with no manual open.
     *
     * @param event The device event; `which` is a device index.
     */
    void onDeviceAdded(const SDL_ControllerDeviceEvent &event);

    /**
     * @brief Handles an SDL_CONTROLLERDEVICEREMOVED event: closes the pad and resets the pad state.
     *
     * A disconnected pad never sends its releases, so the modifier mask, the repeat, and the
     * axis pressed flags are all reset.
     *
     * @param event The device event; `which` is a controller instance id.
     */
    void onDeviceRemoved(const SDL_ControllerDeviceEvent &event);

    /**
     * @brief Handles an SDL_CONTROLLERBUTTONDOWN event.
     *
     * Shoulders set their modifier bit (an in-flight repeat keeps its captured mask); any
     * other button dispatches as a press.
     *
     * @param event The controller button event.
     */
    void onButtonDown(const SDL_ControllerButtonEvent &event);

    /**
     * @brief Handles an SDL_CONTROLLERBUTTONUP event.
     *
     * Shoulders clear their modifier bit; any other button disarms the repeat when it is
     * the repeating input.
     *
     * @param event The controller button event.
     */
    void onButtonUp(const SDL_ControllerButtonEvent &event);

    /**
     * @brief Handles an SDL_CONTROLLERAXISMOTION event: evaluates both directions with hysteresis.
     *
     * @param event The controller axis event.
     */
    void onAxisMotion(const SDL_ControllerAxisEvent &event);

    /**
     * @brief Tells whether a repeat is armed, so the event loop waits with a timeout.
     */
    [[nodiscard]] bool isRepeatArmed() const;

    /**
     * @brief Gets the SDL_GetTicks64 deadline of the armed repeat.
     */
    [[nodiscard]] uint64_t getRepeatDeadline() const;

    /**
     * @brief Fires the armed repeat once its deadline passed, then re-arms at the fast interval.
     *
     * The binding is looked up again on every tick, so a repeated command may rebind its own
     * input. Meant to run after the event poll loop, so fresh events disarm or replace the
     * repeat first.
     */
    void tickRepeat();
};


#endif //CONTROLLER_INPUT_H
