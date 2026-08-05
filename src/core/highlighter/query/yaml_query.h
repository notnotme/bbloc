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
#ifndef YAML_QUERY_H
#define YAML_QUERY_H

#include <string>


static const std::string yaml_query =
R""""(
    (block_mapping_pair key: (_) @keyword)
    (flow_pair key: (_) @keyword)
    [(double_quote_scalar) (single_quote_scalar) (block_scalar) (string_scalar)] @string
    [(integer_scalar) (float_scalar)] @number
    [(boolean_scalar) (null_scalar)] @constant
    (comment) @comment
    [(anchor_name) (alias_name) (tag)] @type
    [(yaml_directive) (tag_directive) (reserved_directive)] @preprocessor
    ["---" "..."] @preprocessor
)"""";


#endif // YAML_QUERY_H
