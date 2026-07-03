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
