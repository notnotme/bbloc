#ifndef JSON_MAPPER_H
#define JSON_MAPPER_H

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
        case 1:     /* {                    */
        case 2:     /* ,                    */
        case 3:     /* }                    */
        case 4:     /* :                    */
        case 5:     /* [                    */
        case 6:     /* ]                    */
        return TokenId::Keyword;
        case 7:     /* "                    */
        case 8:     /* string_content       */
        return TokenId::String;
        case 10:    /* number               */
        return TokenId::Number;
        case 11:     /* true                 */
        case 12:     /* false                */
        case 13:     /* null                 */
        return TokenId::Statement;
        case 14:     /* comment              */
        return TokenId::Comment;
        default:
        return TokenId::None;
    }
}


#endif //JSON_MAPPER_H
