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


ControllerInput::ControllerInput(CommandRunner &commandRunner)
    : m_command_runner(commandRunner),
      m_pad_modifiers(0),
      m_repeat_keycode(SDLK_UNKNOWN),
      m_repeat_modifiers(0),
      m_repeat_deadline(0),
      m_axis_pressed() {}

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
    m_repeat_keycode = SDLK_UNKNOWN;
    m_axis_pressed = {};
}

void ControllerInput::onButtonDown(const SDL_ControllerButtonEvent &event) {
    // Every pad feeds the same state, like a keyboard: no `which` filtering.
    switch (event.button) {
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
            // Shoulders are modifiers; an in-flight repeat keeps its captured mask.
            m_pad_modifiers |= PadInput::KMOD_PAD_L;
        break;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
            m_pad_modifiers |= PadInput::KMOD_PAD_R;
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
        break;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
            m_pad_modifiers &= static_cast<uint16_t>(~PadInput::KMOD_PAD_R);
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
    // A new press always replaces the repeating input.
    m_repeat_keycode = SDLK_UNKNOWN;
    if (m_command_runner.runBoundCommand(keycode, m_pad_modifiers)) {
        // Arm the delay phase, capturing the modifiers held at press time.
        m_repeat_keycode = keycode;
        m_repeat_modifiers = m_pad_modifiers;
        m_repeat_deadline = SDL_GetTicks64() + REPEAT_DELAY_MS;
    }
}

void ControllerInput::release(const SDL_Keycode keycode) {
    if (m_repeat_keycode == keycode) {
        m_repeat_keycode = SDLK_UNKNOWN;
    }
}

bool ControllerInput::isRepeatArmed() const {
    return m_repeat_keycode != SDLK_UNKNOWN;
}

uint64_t ControllerInput::getRepeatDeadline() const {
    return m_repeat_deadline;
}

void ControllerInput::tickRepeat() {
    if (m_repeat_keycode == SDLK_UNKNOWN || SDL_GetTicks64() < m_repeat_deadline) {
        return;
    }

    // Look the binding up again on every tick: a repeated command may rebind its own input.
    m_command_runner.runBoundCommand(m_repeat_keycode, m_repeat_modifiers);
    m_repeat_deadline = SDL_GetTicks64() + REPEAT_INTERVAL_MS;
}
