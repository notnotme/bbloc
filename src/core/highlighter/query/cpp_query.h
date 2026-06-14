#ifndef CPP_QUERY_H
#define CPP_QUERY_H

#include <string>


static const std::string cpp_query =
R""""(
    (auto) @keyword
    (type_identifier) @type
    (primitive_type) @type
    (string_literal) @string
    (number_literal) @number
    (comment) @comment
    (null "nullptr") @constant
    (preproc_def) @preprocessor
    (preproc_include path: (system_lib_string) @string)
    (preproc_include path: (string_literal) @string)
    (preproc_ifdef name: (identifier) @preprocessor)
    [
        "break"
        "catch"
        "class"
        "co_await"
        "co_return"
        "co_yield"
        "concept"
        "constexpr"
        "const"
        "constinit"
        "consteval"
        "default"
        "delete"
        "explicit"
        "export"
        "final"
        "friend"
        "import"
        "module"
        "mutable"
        "namespace"
        "noexcept"
        "new"
        "override"
        "private"
        "protected"
        "public"
        "requires"
        "static"
        "template"
        "throw"
        "try"
        "typename"
        "using"
        "virtual"
        "case"
        "do"
        "else"
        "if"
        "switch"
        "while"
        "return"
    ] @keyword
    ; [
    ; ] @statement
    [
        "#include"
        "#if"
        "#else"
        "#elif"
        "#ifdef"
        "#ifndef"
        "#elifdef"
        "#endif"
    ] @preprocessor
)"""";


#endif // CPP_QUERY_H
