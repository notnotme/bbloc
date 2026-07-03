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
#ifndef PARSER_DESCRIPTOR_H
#define PARSER_DESCRIPTOR_H

#include <string>
#include <unordered_map>
#include <unordered_set>

#include <tree_sitter/api.h>

#include "TokenId.h"


/**
 * @brief Immutable metadata describing a tree-sitter language parser.
 *
 * Owns no tree-sitter resources: the language pointer refers to a static object
 * provided by the language library, so descriptors are safe to keep in static storage.
 */
struct ParserDescriptor final {
    const TSLanguage *language;                         ///< Non-owning pointer to the static tree-sitter language.
    const std::string name;                             ///< Human-readable name of the language.
    const std::string argument_value;                   ///< Argument string usable in the prompt to select this parser.
    const std::unordered_set<std::string> files_format; ///< Set of supported file extensions for the language.
    const std::string query_source;                     ///< Source code of the tree-sitter query for highlighting.
    const std::unordered_map<std::string, TokenId> capture_tokens; ///< Capture name (without '@') mapped to its token classification.
};


#endif //PARSER_DESCRIPTOR_H
