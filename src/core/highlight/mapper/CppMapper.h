#ifndef CPP_MAPPER_H
#define CPP_MAPPER_H

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
        case 2:     /* #include             */
        case 4:     /* #define              */
        case 9:     /* #if                  */
        case 11:    /* #endif               */
        case 12:    /* #ifdef               */
        case 13:    /* #ifndef              */
        case 14:    /* #else                */
        case 15:    /* #elif                */
        case 19:    /* preproc_directive    */
        case 21:    /* defined              */
        return TokenId::Preprocessor;
        case 44:    /* typedef              */
        case 45:    /* virtual              */
        case 46:    /* extern               */
        case 49:    /* using                */
        case 70:    /* unsigned             */
        case 74:    /* static               */
        case 77:    /* register             */
        case 78:    /* inline               */
        case 82:    /* thread_local         */
        case 84:    /* const                */
        case 85:    /* constexpr            */
        case 86:    /* volatile             */
        case 93:    /* mutable              */
        case 96:    /* alignas              */
        case 98:    /* primitive_type       */
        case 99:    /* enum                 */
        case 100:   /* class                */
        case 101:   /* struct               */
        case 102:   /* union                */
        case 144:   /* sizeof               */
        case 148:   /* alignof              */
        case 152:   /* asm                  */
        case 179:   /* auto                 */
        case 177:   /* nullptr              */
        case 180:   /* decltype             */
        case 181:   /* final                */
        case 183:   /* explicit             */
        case 184:   /* typename             */
        case 185:   /* export               */
        case 188:   /* private              */
        case 189:   /* template             */
        case 191:   /* operator             */
        case 193:   /* delete               */
        case 195:   /* friend               */
        case 192:   /* try                  */
        case 196:   /* public               */
        case 197:   /* protected            */
        case 198:   /* noexcept             */
        case 200:   /* namespace            */
        case 205:   /* catch                */
        case 212:   /* new                  */
        case 218:   /* this                 */
        return TokenId::Keyword;
        //case 1:     /** identifier          */
        case 103:   /* if                   */
        case 104:   /* else                 */
        case 105:   /* switch               */
        case 106:   /* case                 */
        case 107:   /* default              */
        case 108:   /* while                */
        case 109:   /* do                   */
        case 110:   /* for                  */
        case 111:   /* return               */
        case 112:   /* break                */
        case 113:   /* continue             */
        case 114:   /* goto                 */
        case 174:   /* true                 */
        case 175:   /* false                */
        case 199:   /* throw                */
        return TokenId::Statement;
        case 159:   /* number_literal       */
        case 219:   /* literal_suffix       */
        return TokenId::Number;
        case 164:   /* '                    */
        case 165:   /* character            */
        case 166:   /* L"                   */
        case 167:   /* u"                   */
        case 169:   /* u8"                  */
        case 170:   /* "                    */
        case 171:   /* string_content       */
        case 173:   /* system_lib_string    */
        case 206:   /* R"                   */
        return TokenId::String;
        case 178:   /* comment */
        return TokenId::Comment;
        default:
        return TokenId::None;
    }
}


#endif //CPP_MAPPER_H
