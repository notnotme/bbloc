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
#ifndef U16_STRING_MAP_H
#define U16_STRING_MAP_H

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>


/**
 * @brief Transparent hash functor enabling heterogeneous lookups in UTF-16 string-keyed maps.
 *
 * Hashes std::u16string keys and std::u16string_view lookups identically, so a map using it
 * can be queried with a view without materializing a temporary std::u16string.
 */
struct U16StringViewHash final {
    using is_transparent = void; ///< Opts the map into heterogeneous lookups.

    /**
     * @brief Hashes a UTF-16 text.
     *
     * @param text The text to hash.
     * @return The hash of the text.
     */
    [[nodiscard]] size_t operator()(const std::u16string_view text) const noexcept {
        return std::hash<std::u16string_view>{}(text);
    }
};

/**
 * @brief Unordered map keyed by UTF-16 strings, queryable with string views without allocating.
 *
 * @tparam TValue The mapped value type.
 */
template <typename TValue>
using U16StringMap = std::unordered_map<std::u16string, TValue, U16StringViewHash, std::equal_to<>>;


#endif //U16_STRING_MAP_H
