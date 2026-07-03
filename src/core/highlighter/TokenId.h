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
#ifndef TOKEN_ID_H
#define TOKEN_ID_H


/**
 * @brief Represents the classification of a token for syntax highlighting or parsing.
 *
 * TokenId used to identify code fragments (e.g., comment, keyword, string literal).
 */
enum class TokenId {
    None,         ///< Default value for unclassified or irrelevant tokens.
    Keyword,      ///< A language keyword (e.g., class, public, protected, void).
    Statement,    ///< A structural statement or control flow construct (e.g., if, else, switch, return).
    String,       ///< A string literal.
    Number,       ///< A numeric literal (integer, float, etc.).
    Comment,      ///< A comment (single-line, multi-line, or documentation).
    Preprocessor, ///< A preprocessor directive (e.g., #include, #define).
    Type,         ///< A type identifier.
    Constant,     ///< A constant or macro.
    Function,     ///< A function or method name (definition or call).
    Variable      ///< A variable, parameter, or member field.
};


#endif //TOKEN_ID_H
