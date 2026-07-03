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
#ifndef HIGH_LIGHT_ID_H
#define HIGH_LIGHT_ID_H


/**
 * @brief Identifies the syntax highlighting mode.
 *
 * This enum is used to describe the highlight for a given mode.
 */
enum class HighLightId {
    None, ///< No highlighting applied.
    Cpp,  ///< C++ syntax highlighting.
    Json  ///< JSON syntax highlighting.
};

#endif //HIGH_LIGHT_ID_H
