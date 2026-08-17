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
#include "CommandLine.h"


void CommandLine::tokenize(const std::u16string_view input, std::vector<std::u16string_view> &tokens) {
    tokens.clear();
    std::size_t start = 0;
    std::size_t index = 0;
    while (index < input.length()) {
        constexpr auto space_delimiter = U' ';
        constexpr auto quote_delimiter = U'"';

        // Skip blank spaces
        if (input[index] == space_delimiter) {
            ++index;
            continue;
        }

        start = index;
        if (input[index] == quote_delimiter) {
            // skip opening quote
            ++start;
            ++index;
            while (index < input.length() && input[index] != quote_delimiter) {
                ++index;
            }

            if (index < input.length()) {
                tokens.emplace_back(input.substr(start, index - start));
                // skip closing quote
                ++index;
            } else {
                // Unterminated quote, take until the end
                tokens.emplace_back(input.substr(start));
                break;
            }
        } else {
            // Unquoted word
            while (index < input.size() && input[index] != space_delimiter) {
                ++index;
            }
            tokens.emplace_back(input.substr(start, index - start));
        }
    }
}

std::vector<std::u16string_view> CommandLine::split(const std::u16string_view input, const char16_t delimiter) {
    std::vector<std::u16string_view> parts;
    auto start = input.find_first_not_of(delimiter);
    while (start != std::u16string_view::npos) {
        const auto end = input.find(delimiter, start);
        parts.emplace_back(input.substr(start, end - start));
        start = input.find_first_not_of(delimiter, end);
    }

    return parts;
}
