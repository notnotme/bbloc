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
#include "ViewState.h"


ViewState::ViewState()
    : m_position_x(0),
      m_position_y(0),
      m_width(0),
      m_height(0) {}

int32_t ViewState::getPositionX() const {
    return m_position_x;
}

int32_t ViewState::getPositionY() const {
    return m_position_y;
}

int32_t ViewState::getWidth() const {
    return m_width;
}

int32_t ViewState::getHeight() const {
    return m_height;
}

void ViewState::setPosition(const int32_t x, const int32_t y) {
    m_position_x = x;
    m_position_y = y;
}

void ViewState::setSize(const int32_t width, const int32_t height) {
    m_width = width;
    m_height = height;
}