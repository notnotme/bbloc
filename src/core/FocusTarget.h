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
#ifndef FOCUS_TARGET_H
#define FOCUS_TARGET_H


/**
 * @brief Represents the view the keyboard focus is on.
 *
 * Only the keyboard: which typing — physical or on-screen-keyboard-synthesized — edits
 * the buffer and which edits the command line. Who owns the game pad is a separate fact,
 * tracked by OskState::hasPadFocus().
 */
enum class FocusTarget {
    Editor, ///< Editor view is focused.
    Prompt  ///< Prompt view is focused.
};


#endif //FOCUS_TARGET_H
