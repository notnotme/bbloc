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
#ifndef CPP_QUERY_H
#define CPP_QUERY_H

#include <string>


static const std::string cpp_query =
R""""(
    ; ---- Comments / literals ----
    (comment) @comment
    (string_literal) @string
    (raw_string_literal) @string
    (char_literal) @string
    (number_literal) @number

    ; ---- Preprocessor ----
    (preproc_def name: (identifier) @constant)
    (preproc_function_def name: (identifier) @function)
    (preproc_ifdef name: (identifier) @constant)
    (preproc_defined (identifier) @constant)
    (preproc_include path: (system_lib_string) @string)
    (preproc_directive) @preprocessor
    [
        "#include" "#define" "#if" "#else" "#elif"
        "#ifdef" "#ifndef" "#elifdef" "#endif" "defined"
    ] @preprocessor

    ; ---- Functions & methods: definitions ----
    (function_declarator declarator: (identifier) @function)
    (function_declarator declarator: (field_identifier) @function)
    (function_declarator declarator: (qualified_identifier name: (identifier) @function))
    (function_declarator declarator: (destructor_name) @function)
    (function_declarator declarator: (operator_name) @function)

    ; ---- Functions & methods: calls ----
    (call_expression function: (identifier) @function)
    (call_expression function: (field_expression field: (field_identifier) @function))
    (call_expression function: (qualified_identifier name: (identifier) @function))
    (template_function name: (identifier) @function)
    (template_method name: (field_identifier) @function)

    ; ---- Types ----
    (type_identifier) @type
    (primitive_type) @type
    (sized_type_specifier) @type
    (namespace_identifier) @type
    (auto) @keyword

    ; ---- Constants ----
    (this) @constant
    (null) @constant
    (true) @constant
    (false) @constant
    (enumerator name: (identifier) @constant)

    ; ---- Control flow ----
    [
        "if" "else" "switch" "case" "default" "do" "while" "for"
        "break" "continue" "return" "goto"
        "try" "catch" "throw"
        "co_return" "co_await" "co_yield"
    ] @statement

    ; ---- Keywords ----
    [
        "class" "struct" "enum" "union" "typedef" "namespace" "using"
        "template" "typename" "concept" "requires" "decltype"
        "public" "private" "protected" "friend"
        "virtual" "override" "final" "explicit" "operator"
        "static" "extern" "inline" "thread_local" "register"
        "const" "constexpr" "consteval" "constinit" "mutable" "volatile"
        "noexcept" "static_assert" "sizeof"
        "new" "delete"
        "signed" "unsigned" "long" "short"
        "export" "import" "module"
    ] @keyword

    ; ---- Variables (generic catch-alls, must stay last) ----
    (field_identifier) @variable
    (identifier) @variable
)"""";


#endif // CPP_QUERY_H
