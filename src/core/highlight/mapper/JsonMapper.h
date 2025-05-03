#ifndef JSON_MAPPER_H
#define JSON_MAPPER_H

#include <cstdint>

#include "../TokenId.h"


/**
 * @brief Maps a Tree-sitter JSON symbol ID to a corresponding TokenId.
 *
 * Used to classify JSON tokens (e.g., keywords, strings, object) for syntax highlighting.
 *
 * @param symbol The Tree-sitter JSON symbol ID.
 * @return The corresponding TokenId classification.
 */
inline TokenId mapJsonToken(const uint16_t symbol) {
    switch (symbol) {
        default: return TokenId::None;
    }
}


#endif //JSON_MAPPER_H
