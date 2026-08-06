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
#include "OskState.h"

#include "../platform/Platform.h"


OskState::OskState()
    : m_visible(false),
      m_page(0),
      p_layout(&OskLayout::defaultLayout()),
      m_sticky(),
      m_cursor_row(2),
      m_cursor_col(5),
      m_pressed_row(-1),
      m_pressed_col(0),
      m_press_time(0) {
    // Follow the system keyboard layout where the platform exposes one (Switch console
    // setting); "osk layout" overrides it at runtime.
    if (const auto *layout = OskLayout::findLayout(Platform::keyboardLayout()); layout != nullptr) {
        p_layout = layout;
    }
}

bool OskState::isVisible() const {
    return m_visible;
}

int32_t OskState::getPage() const {
    return m_page;
}

const OskLayout::Layout &OskState::getLayout() const {
    return *p_layout;
}

OskState::StickyState OskState::getSticky(const StickyModifier modifier) const {
    return m_sticky[static_cast<std::size_t>(modifier)];
}

uint16_t OskState::stickyModifierMask() const {
    auto mask = 0;
    if (getSticky(StickyModifier::Ctrl) != StickyState::Idle) {
        mask |= KMOD_LCTRL;
    }

    if (getSticky(StickyModifier::Shift) != StickyState::Idle) {
        mask |= KMOD_LSHIFT;
    }

    if (getSticky(StickyModifier::Alt) != StickyState::Idle) {
        mask |= KMOD_LALT;
    }

    if (getSticky(StickyModifier::AltGr) != StickyState::Idle) {
        mask |= KMOD_RALT;
    }

    return static_cast<uint16_t>(mask);
}

int32_t OskState::getCursorRow() const {
    return m_cursor_row;
}

int32_t OskState::getCursorCol() const {
    return m_cursor_col;
}

int32_t OskState::getPressedRow() const {
    return m_pressed_row;
}

int32_t OskState::getPressedCol() const {
    return m_pressed_col;
}

uint64_t OskState::getPressTime() const {
    return m_press_time;
}

InputRepeater &OskState::getRepeater() {
    return m_repeater;
}

const InputRepeater &OskState::getRepeater() const {
    return m_repeater;
}

void OskState::setVisible(const bool visible) {
    m_visible = visible;
}

void OskState::setPage(const int32_t page) {
    m_page = page;
}

void OskState::setLayout(const OskLayout::Layout &layout) {
    p_layout = &layout;
}

void OskState::setSticky(const StickyModifier modifier, const StickyState state) {
    m_sticky[static_cast<std::size_t>(modifier)] = state;
}

void OskState::releaseLatched() {
    for (auto &sticky : m_sticky) {
        if (sticky == StickyState::Latched) {
            sticky = StickyState::Idle;
        }
    }
}

void OskState::setCursor(const int32_t row, const int32_t col) {
    m_cursor_row = row;
    m_cursor_col = col;
}

void OskState::setPressed(const int32_t row, const int32_t col, const uint64_t pressTime) {
    m_pressed_row = row;
    m_pressed_col = col;
    m_press_time = pressTime;
}

void OskState::clearPressed() {
    m_pressed_row = -1;
    m_pressed_col = 0;
    m_press_time = 0;
}

void OskState::resetInteraction() {
    m_sticky = {};
    m_page = 0;
    clearPressed();
    m_repeater.disarm();
}
