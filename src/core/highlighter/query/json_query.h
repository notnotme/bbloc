#ifndef JSON_QUERY_H
#define JSON_QUERY_H

#include <string>


static const std::string json_query =
R""""(
    (pair key: (_) @keyword)
    (string) @string
    (number) @number
    (comment) @comment
    [(null) (true) (false)] @constant
    ; TODO (escape_sequence) @escape
)"""";


#endif // JSON_QUERY_H
