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
#ifndef DIMENSION_ID_H
#define DIMENSION_ID_H


/**
 * @brief Enumeration of dimension identifiers used for layout and spacing.
 *
 * These identifiers represent configurable values (in pixels or units)
 * for padding, indicators, borders, and logical sizes like tab width or page scrolling.
 */
enum class DimensionId {
    PaddingWidth,   ///< Padding applied to the sides of content, in pixels.
    IndicatorWidth, ///< Width of the cursor or indicator, in pixels.
    BorderSize,     ///< Thickness of borders between UI components, in pixels.
    TabToSpace,     ///< Number of spaces to display per tab character.
    PageUpDown      ///< Number of lines to scroll on PageUp/PageDown events.
};


#endif //DIMENSION_ID_H
