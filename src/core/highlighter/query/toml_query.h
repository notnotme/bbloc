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
#ifndef TOML_QUERY_H
#define TOML_QUERY_H

#include <string>


static const std::string toml_query =
R""""(
    (table [(bare_key) (dotted_key) (quoted_key)] @type)
    (table_array_element [(bare_key) (dotted_key) (quoted_key)] @type)
    (pair [(bare_key) (dotted_key) (quoted_key)] @keyword)
    (string) @string
    [(integer) (float)] @number
    [(offset_date_time) (local_date_time) (local_date) (local_time)] @number
    (boolean) @constant
    (comment) @comment
)"""";


#endif // TOML_QUERY_H
