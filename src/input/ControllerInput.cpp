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
#include "ControllerInput.h"


ControllerInput::ControllerInput(CommandRunner &commandRunner, CursorContextManager &contextManager, Osk &osk, OskState &oskState)
    : m_command_runner(commandRunner),
      m_context_manager(contextManager),
      m_osk(osk),
      m_osk_state(oskState),
      m_pad_modifiers(0),
      m_axis_pressed() {}

bool ControllerInput::dispatch(const SDL_Keycode keycode, const uint16_t modifiers) {
    // While the on-screen keyboard has the pad focus, d-pad/A/B drive its key cursor
    // instead of running bindings; anything it does not handle falls through unchanged.
    auto &context = m_context_manager.active();
    if (m_osk_state.isVisible()) {
        // Lazy acquisition: the visible OSK takes the pad (and shows its key cursor) only
        // when a pad actually navigates it, so mouse and touch users never see the cursor.
        // Only the pad moves: the keyboard focus stays where it is, so the OSK types into
        // an active prompt as well as the editor.
        const auto is_direction = keycode == PadInput::fromButton(SDL_CONTROLLER_BUTTON_DPAD_UP)
            || keycode == PadInput::fromButton(SDL_CONTROLLER_BUTTON_DPAD_DOWN)
            || keycode == PadInput::fromButton(SDL_CONTROLLER_BUTTON_DPAD_LEFT)
            || keycode == PadInput::fromButton(SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
        const auto acquires = is_direction || keycode == PadInput::fromButton(SDL_CONTROLLER_BUTTON_A);
        if (!m_osk_state.hasPadFocus() && acquires) {
            m_osk_state.setPadFocus(true);
        }

        if (m_osk_state.hasPadFocus() && m_osk.onPadInput(context, m_osk_state, keycode)) {
            // Only held directions auto-repeat: A would re-tap keys, B would re-leave.
            return is_direction;
        }
    }

    return m_command_runner.runBoundCommand(keycode, modifiers);
}

void ControllerInput::onDeviceAdded(const SDL_ControllerDeviceEvent &event) {
    // `which` is a device index here; the open handles are kept for the matching REMOVED.
    // The handles left open at exit are released by SDL_Quit.
    if (auto *const controller = SDL_GameControllerOpen(event.which)) {
        m_controllers.push_back(controller);
    }
}

void ControllerInput::onDeviceRemoved(const SDL_ControllerDeviceEvent &event) {
    // `which` is a controller instance id here, not a device index.
    if (auto *const controller = SDL_GameControllerFromInstanceID(event.which)) {
        SDL_GameControllerClose(controller);
        std::erase(m_controllers, controller);
    }

    // A disconnected pad never sends its releases: reset the whole live pad state.
    m_pad_modifiers = 0;
    publishOskModifiers();
    m_repeater.disarm();
    m_axis_pressed = {};
}

void ControllerInput::publishOskModifiers() {
    const auto shift = (m_pad_modifiers & PadInput::KMOD_PAD_L) != 0 ? KMOD_LSHIFT : 0;
    // The state write is unconditional so the mask stays truthful: showing the on-screen
    // keyboard while a shoulder is already held must find the Shift layer already set. The
    // repaint is not: the key labels resolve under the mask, so only a visible strip has
    // anything to redraw — otherwise every shoulder press would force a frame for nothing.
    m_osk_state.setLiveModifiers(static_cast<uint16_t>(shift));
    if (m_osk_state.isVisible()) {
        m_context_manager.active().wants_redraw = true;
    }
}

void ControllerInput::onButtonDown(const SDL_ControllerButtonEvent &event) {
    // Every pad feeds the same state, like a keyboard: no `which` filtering.
    switch (event.button) {
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
            // Shoulders are modifiers; an in-flight repeat keeps its captured mask.
            m_pad_modifiers |= PadInput::KMOD_PAD_L;
            publishOskModifiers();
        break;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
            m_pad_modifiers |= PadInput::KMOD_PAD_R;
            publishOskModifiers();
        break;
        default:
            press(PadInput::fromButton(static_cast<SDL_GameControllerButton>(event.button)));
        break;
    }
}

void ControllerInput::onButtonUp(const SDL_ControllerButtonEvent &event) {
    switch (event.button) {
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
            m_pad_modifiers &= static_cast<uint16_t>(~PadInput::KMOD_PAD_L);
            publishOskModifiers();
        break;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
            m_pad_modifiers &= static_cast<uint16_t>(~PadInput::KMOD_PAD_R);
            publishOskModifiers();
        break;
        default:
            release(PadInput::fromButton(static_cast<SDL_GameControllerButton>(event.button)));
        break;
    }
}

void ControllerInput::onAxisMotion(const SDL_ControllerAxisEvent &event) {
    const auto axis = static_cast<SDL_GameControllerAxis>(event.axis);
    // Widen before negating: -SDL_MIN_SINT16 does not fit an int16. Triggers only ever go
    // positive, so their negative direction simply never crosses the press threshold.
    const auto value = static_cast<int32_t>(event.value);
    updateAxisDirection(axis, false, -value);
    updateAxisDirection(axis, true, value);
}

void ControllerInput::updateAxisDirection(const SDL_GameControllerAxis axis, const bool positive, const int32_t magnitude) {
    auto &pressed = m_axis_pressed[axis][positive ? 1 : 0];
    if (!pressed && magnitude >= AXIS_PRESS_THRESHOLD) {
        // Press transition: behaves like a button press of the direction pseudo-button.
        pressed = true;
        press(PadInput::fromAxis(axis, positive));
    } else if (pressed && magnitude < AXIS_RELEASE_THRESHOLD) {
        // Release transition; the gap between the two thresholds absorbs the flutter.
        pressed = false;
        release(PadInput::fromAxis(axis, positive));
    }
}

void ControllerInput::press(const SDL_Keycode keycode) {
    // The single choke point every pad press flows through, buttons and axis directions
    // alike, and it runs before the dispatch — so the message a bound command produces
    // survives the press that ran it. Repeat ticks go straight to dispatch, so a held
    // input never dismisses again.
    m_command_runner.dismissMessage();

    // A new press always replaces the repeating input.
    m_repeater.disarm();
    if (dispatch(keycode, m_pad_modifiers)) {
        // Arm the delay phase, capturing the modifiers held at press time.
        m_repeater.arm(keycode, m_pad_modifiers);
    }
}

void ControllerInput::release(const SDL_Keycode keycode) {
    m_repeater.disarmIf(keycode);
}

bool ControllerInput::isRepeatArmed() const {
    return m_repeater.isArmed();
}

uint64_t ControllerInput::getRepeatDeadline() const {
    return m_repeater.getDeadline();
}

void ControllerInput::tickRepeat() {
    if (!m_repeater.isDue()) {
        return;
    }

    // Route again on every tick: a repeated command may rebind its own input, and the
    // on-screen keyboard cursor repeats through the same path.
    dispatch(m_repeater.getCode(), m_repeater.getModifiers());
    m_repeater.rearm();
}
