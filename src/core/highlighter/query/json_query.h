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
#ifndef JSON_QUERY_H
#define JSON_QUERY_H

#include <string>


static const std::string json_query =
R""""(
    (pair key: (_) @keyword)
    (string) @string
    (number) @number
    (comment) @comment
    [(null) (true) (false)] @constant
    ; TODO (escape_sequence) @escape
)"""";


#endif // JSON_QUERY_H
