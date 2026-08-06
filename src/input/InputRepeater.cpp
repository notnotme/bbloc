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
#include "InputRepeater.h"

#include <SDL_timer.h>


InputRepeater::InputRepeater()
    : m_armed(false),
      m_code(0),
      m_modifiers(0),
      m_deadline(0) {}

void InputRepeater::arm(const int32_t code, const uint16_t modifiers) {
    m_armed = true;
    m_code = code;
    m_modifiers = modifiers;
    m_deadline = SDL_GetTicks64() + REPEAT_DELAY_MS;
}

void InputRepeater::disarm() {
    m_armed = false;
}

void InputRepeater::disarmIf(const int32_t code) {
    if (m_armed && m_code == code) {
        m_armed = false;
    }
}

void InputRepeater::rearm() {
    m_deadline = SDL_GetTicks64() + REPEAT_INTERVAL_MS;
}

bool InputRepeater::isArmed() const {
    return m_armed;
}

bool InputRepeater::isDue() const {
    return m_armed && SDL_GetTicks64() >= m_deadline;
}

int32_t InputRepeater::getCode() const {
    return m_code;
}

uint16_t InputRepeater::getModifiers() const {
    return m_modifiers;
}

uint64_t InputRepeater::getDeadline() const {
    return m_deadline;
}
