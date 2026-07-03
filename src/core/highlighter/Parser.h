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
#ifndef PARSER_H
#define PARSER_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <tree_sitter/api.h>

#include "ParserDescriptor.h"
#include "TokenId.h"


/**
 * @brief Owns the compiled tree-sitter highlight query for a single language.
 *
 * The query is compiled from the descriptor at construction and released at destruction.
 * Copying is deleted (and no move operations are provided), so the query can never be double-freed.
 */
class Parser final {
private:
    /** Non-owning reference to the descriptor; descriptors have static storage (ParserCatalog). */
    const ParserDescriptor &m_descriptor;

    /** Owning pointer to the compiled tree-sitter query. */
    TSQuery *p_query;

    /** Token classification for each query capture, indexed by capture index. */
    std::vector<TokenId> m_capture_token_ids;

public:
    /** @brief Deleted copy constructor. */
    Parser(const Parser &) = delete;

    /** @brief Deleted copy assignment operator. */
    Parser &operator=(const Parser &) = delete;

    /** @brief Free the compiled query. */
    ~Parser();

    /**
     * @brief Compiles the descriptor's highlight query.
     *
     * @param descriptor Language metadata whose query source is compiled.
     * @throws std::runtime_error If the query source fails to compile.
     */
    explicit Parser(const ParserDescriptor &descriptor);

    /** @return Non-owning pointer to the tree-sitter language definition. */
    [[nodiscard]] const TSLanguage *getLanguage() const;

    /** @return Non-owning pointer to the compiled tree-sitter query. */
    [[nodiscard]] const TSQuery *getQuery() const;

    /**
     * @brief Returns the token classification of a query capture.
     *
     * @param captureIndex Capture index reported by a query match.
     * @return TokenId associated with the capture, or TokenId::None if unmapped.
     */
    [[nodiscard]] TokenId getTokenId(uint32_t captureIndex) const;

    /** @return Human-readable name of the language. */
    [[nodiscard]] std::string_view getName() const;

    /** @return Argument string usable in the prompt to select this parser. */
    [[nodiscard]] std::string_view getArgumentValue() const;

    /**
     * @brief Checks whether this parser supports a given file extension.
     *
     * @param extension File extension to query (including the dot).
     * @return true if the extension is supported.
     */
    [[nodiscard]] bool supportsExtension(const std::string &extension) const;
};


#endif //PARSER_H
