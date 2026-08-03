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
#ifndef PARSER_CATALOG_H
#define PARSER_CATALOG_H

#include <string_view>
#include <unordered_map>

#include "HighLightId.h"
#include "Parser.h"
#include "ParserDescriptor.h"


/**
 * @brief Static-only registry of the available language parsers.
 *
 * Houses the immutable descriptor table and the Parser instances compiled from it.
 * Both live in function-local statics, so no tree-sitter work happens at
 * static-initialization time and the queries are compiled only once per process.
 */
class ParserCatalog final {
public:
    /** @brief Deleted constructor; this class is static-only. */
    ParserCatalog() = delete;

    /** @return Immutable descriptor table indexed by highlighting mode. */
    [[nodiscard]] static const std::unordered_map<HighLightId, ParserDescriptor> &getDescriptors();

    /**
     * @brief Returns the compiled parsers, compiling them on the first call.
     *
     * Compiling a highlight query walks the whole parse table of its language, so the parsers
     * live in a function-local static shared by every HighLighter instead of being rebuilt per
     * open buffer. Parser is immutable once built, only const accessors are ever called on it.
     *
     * @return Immutable map of compiled parsers indexed by highlighting mode.
     * @throws std::runtime_error If a highlight query fails to compile.
     */
    [[nodiscard]] static const std::unordered_map<HighLightId, Parser> &getParsers();

    /**
     * @brief Finds the highlighting mode matching a file extension.
     *
     * @param extension File extension (including the dot), e.g. ".cpp".
     * @return The matching mode, or HighLightId::None if nothing matches.
     */
    [[nodiscard]] static HighLightId findModeByExtension(std::string_view extension);
};


#endif //PARSER_CATALOG_H
