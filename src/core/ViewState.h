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
#ifndef VIEW_STATE_H
#define VIEW_STATE_H

#include <cstdint>


/**
 * @brief Abstract base class representing a UI view state.
 *
 * This class is designed to be inherited by concrete view state implementations.
 */
class ViewState {
protected:
    /** X offset of the InfoBar within the window (in pixels). */
    int16_t m_position_x;

    /** Y offset of the InfoBar within the window (in pixels). */
    int16_t m_position_y;

    /** Width of the InfoBar (in pixels). */
    uint16_t m_width;

    /** Height of the InfoBar (in pixels). */
    uint16_t m_height;

public:
    /** @brief Deleted copy constructor. */
    ViewState(const ViewState &) = delete;

    /** @brief Deleted copy assignment operator. */
    ViewState &operator=(const ViewState &) = delete;

    /** @brief Create a ViewState with default values. */
    explicit ViewState();

    /** @brief For inheritance */
    virtual ~ViewState() = default;

    /** @brief Returns the X position of the view. */
    [[nodiscard]] int16_t getPositionX() const;

    /** @brief Returns the Y position of the view. */
    [[nodiscard]] int16_t getPositionY() const;

    /** @brief Returns the width of the view. */
    [[nodiscard]] uint16_t getWidth() const;

    /** @brief Returns the height of the view. */
    [[nodiscard]] uint16_t getHeight() const;

    /**
     * @brief Sets the position of the info bar view.
     *
     * @param x The new X position.
     * @param y The new Y position.
     */
    void setPosition(int16_t x, int16_t y);

    /**
     * @brief Sets the size of the info bar view.
     *
     * @param width The new width in pixels.
     * @param height The new height in pixels.
     */
    void setSize(uint16_t width, uint16_t height);
};


#endif //VIEW_STATE_H
