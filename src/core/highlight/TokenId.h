#ifndef TOKEN_ID_H
#define TOKEN_ID_H


/**
 * @brief Represents the classification of a token for syntax highlighting or parsing.
 *
 * TokenId used to identify code fragments (e.g., comment, keyword, string literal).
 */
enum class TokenId {
    None,         ///< Default value for unclassified or irrelevant tokens.
    Comment,      ///< A comment (single-line, multi-line, or documentation).
    Preprocessor, ///< A preprocessor directive (e.g., #include, #define).
    String,       ///< A string literal.
    Number,       ///< A numeric literal (integer, float, etc.).
    Keyword,      ///< A language keyword (e.g., if, class, return).
    Statement     ///< A structural statement or control flow construct.
};


#endif //TOKEN_ID_H
