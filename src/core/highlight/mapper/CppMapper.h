#ifndef CPP_MAPPER_H
#define CPP_MAPPER_H

#include <cstdint>

#include "../TokenId.h"


/**
 * @brief Maps a Tree-sitter C++ symbol ID to a corresponding TokenId.
 *
 * Used to classify C++ tokens (e.g., keywords, strings, comments) for syntax highlighting.
 *
 * @param symbol The Tree-sitter C++ symbol ID.
 * @return The corresponding TokenId classification.
 */
inline TokenId mapCppToken(const uint16_t symbol) {
    switch (symbol) {
        case 2:
        case 4:
        case 9:
        case 11:
        case 13:
        case 14:
        case 19:
        case 21: return TokenId::Preprocessor;
        case 98:
        case 100:
        case 106:
        case 112:
        case 179:
        case 382: return TokenId::Keyword;
        case 84:
        case 108:
        case 111: return TokenId::Statement;
        case 159: return TokenId::Number;
        case 167:
        case 170:
        case 171:
        case 173: return TokenId::String;
        case 178: return TokenId::Comment;
        default: return TokenId::None;
    }
}


#endif //CPP_MAPPER_H
