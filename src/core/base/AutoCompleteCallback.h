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
#ifndef AUTO_COMPLETE_CALLBACK_H
#define AUTO_COMPLETE_CALLBACK_H

#include <functional>
#include <string_view>


/**
 * @brief Template alias for an auto-completion callback function.
 *
 * This callback is called for each item that potentially matches with the input.
 *
 * @param input A string view representing the item being returned.
 */
using AutoCompleteCallback = std::function<
    void(std::u16string_view input)
>;


#endif //AUTO_COMPLETE_CALLBACK_H
