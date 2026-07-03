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
#ifndef CVAR_CALLBACK_H
#define CVAR_CALLBACK_H

#include <functional>


/**
 * @brief Callback function type invoked when a configuration variable (CVar) is modified.
 *
 * This callback is called after a CVar's value is successfully changed, allowing
 * the application to respond to the change (e.g., updating UI, triggering side effects).
 */
using CVarCallback = std::function<
    void()
>;


#endif //CVAR_CALLBACK_H
