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
#ifndef MARKDOWN_QUERY_H
#define MARKDOWN_QUERY_H

#include <string>


// Block grammar only (tree_sitter_markdown): inline emphasis/links live in the separate
// tree-sitter-markdown-inline grammar, which would need injection support in HighLighter.
static const std::string markdown_query =
R""""(
    (atx_heading) @keyword
    (setext_heading) @keyword
    (fenced_code_block) @string
    (indented_code_block) @string
    (block_quote) @comment
    (thematic_break) @preprocessor
    [(list_marker_minus) (list_marker_plus) (list_marker_star) (list_marker_dot) (list_marker_parenthesis)] @statement
    [(task_list_marker_checked) (task_list_marker_unchecked)] @constant
    (link_reference_definition (link_label) @type (link_destination) @variable)
    (html_block) @preprocessor
    (pipe_table_header) @type
    (pipe_table_delimiter_row) @preprocessor
    (minus_metadata) @comment
    (plus_metadata) @comment
)"""";


#endif // MARKDOWN_QUERY_H
