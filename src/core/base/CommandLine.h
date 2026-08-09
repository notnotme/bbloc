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
#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

#include <string_view>
#include <vector>


/**
 * @brief Static-only syntax of a command line: the split into a command name and its arguments.
 *
 * Every command reaches the application through here — typed at the prompt, bound to a key, or read
 * from an exec script — which is why this depends on nothing but the standard library. The tokens
 * are views into the input string, and callers rely on that: AutoCompleteCommand measures the
 * argument being completed by the distance between a token and the start of the line.
 */
class CommandLine final {
public:
    /** @brief Deleted constructor; this class is static-only. */
    CommandLine() = delete;

    /**
     * @brief Tokenizes a UTF-16 input string for command parsing. Splits the input into a list of arguments.
     *
     * Quoted arguments are preserved as single tokens.
     *
     * @param input The UTF-16 input string.
     * @param tokens Receives the UTF-16 views of each argument token; cleared first, reusing its capacity.
     */
    static void tokenize(std::u16string_view input, std::vector<std::u16string_view> &tokens);

    /**
     * @brief Split a UTF-16 input string.
     *
     * @param input The UTF-16 input string.
     * @param delimiter The delimiter to use to split the string apart.
     * @return Vector of UTF-16 views representing each part.
     */
    [[nodiscard]] static std::vector<std::u16string_view> split(std::u16string_view input, char16_t delimiter);
};


#endif //COMMAND_LINE_H
