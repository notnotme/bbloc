#include "HighLighter.h"

#include <ranges>
#include <utf8.h>

#include <tree_sitter/tree-sitter-cpp.h>
#include <tree_sitter/tree-sitter-json.h>

#include "query/cpp_query.h"
#include "query/json_query.h"


const std::unordered_map<HighLightId, HighLighter::Parser> HighLighter::PARSERS = {
    { HighLightId::Json, {
        .language           = tree_sitter_json(),
        .name               = "JSON",
        .argument_value     = "json",
        .files_format       = {".json", ".JSON"},
        .query_source       = json_query,
        .query              = nullptr
    }},
    { HighLightId::Cpp, {
        .language           = tree_sitter_cpp(),
        .name               = "C++",
        .argument_value     = "cpp",
        .files_format       = {".c", ".C", ".cc", ".CC", ".cpp", ".CPP", ".h", ".H", ".hpp", ".HPP", ".cxx", ".CXX"},
        .query_source       = cpp_query,
        .query              = nullptr
    }}
};

HighLighter::HighLighter(Cursor &cursor)
    : m_cursor(cursor),
      p_current_parser(nullptr),
      p_ts_parser(ts_parser_new()),
      p_ts_tree(nullptr),
      p_ts_query_cursor(ts_query_cursor_new()),
      m_input(this, inputCallback, TSInputEncodingUTF16LE),
      m_high_light(HighLightId::None),
      m_is_dirty(false) {

    // Compile queries
    for (auto& [id, parser] : const_cast<std::unordered_map<HighLightId, Parser>&>(PARSERS)) {
        uint32_t error_offset;
        TSQueryError error_type;
        parser.query = ts_query_new(
            parser.language,
            parser.query_source.data(),
            parser.query_source.length(),
            &error_offset,
            &error_type
        );
        if (parser.query == nullptr) {
            // In a real app we might want to handle this better, but for now:
            throw std::runtime_error("Tree-sitter query error");
        }
    }
}

HighLighter::~HighLighter() {
    // Cleanup tree
    if (p_ts_tree != nullptr) {
        ts_tree_delete(p_ts_tree);
        p_ts_tree = nullptr;
    }

    // Cleanup parser
    ts_parser_delete(p_ts_parser);
    p_ts_parser = nullptr;
    p_current_parser = nullptr;

    // Cleanup query cursor
    ts_query_cursor_delete(p_ts_query_cursor);
    p_ts_query_cursor = nullptr;

    // Cleanup queries
    for (auto& [id, parser] : const_cast<std::unordered_map<HighLightId, Parser>&>(PARSERS)) {
        if (parser.query != nullptr) {
            ts_query_delete(parser.query);
            parser.query = nullptr;
        }
    }
}

void HighLighter::setMode(const HighLightId highLight) {
    m_high_light = highLight;
    m_is_dirty = true;

    auto *old_language = ts_parser_language(p_ts_parser);
    if (old_language != nullptr) {
        // Free dynamic allocation done by the language
        ts_language_delete(old_language);
    }

    if (highLight == HighLightId::None) {
        // There is no need to keep the old mode
        p_current_parser = nullptr;
    } else {
        // Set new mode
        const auto &parser = PARSERS.at(highLight);
        if (! ts_parser_set_language(p_ts_parser, parser.language)) {
            throw std::runtime_error("Could not set parser for language highlight");
        }

        p_current_parser = &parser;
    }

    if (p_ts_tree != nullptr) {
        // Delete the old tree, as tree-sitter may need to reparse it from scratch
        ts_tree_delete(p_ts_tree);
        p_ts_tree = nullptr;
    }

    m_line_cache.clear();
}

void HighLighter::setMode(const std::string_view extension) {
    for (const auto &[id, parser] : PARSERS) {
        if (parser.files_format.contains(extension.data())) {
            // Found a match!
            setMode(id);
            return;
        }
    }

    // Fall back to none
    setMode(HighLightId::None);
}

std::string_view HighLighter::getModeString() const {
    if (m_high_light == HighLightId::None) {
        // None is TEXT
        return "TEXT";
    }

    return p_current_parser->name;
}

void HighLighter::parse() {
    if (p_current_parser != nullptr && m_is_dirty) {
        // Reuse the old tree to parse create a new one, if p_ts_tree is nullptr, start from scratch
        auto *new_tree = ts_parser_parse(p_ts_parser, p_ts_tree, m_input);
        // Delete the old tree and set the new as the current one
        if (p_ts_tree != nullptr) {
            ts_tree_delete(p_ts_tree);
        }
        p_ts_tree = new_tree;
        updateCache();

        m_is_dirty = false;
    }
}

void HighLighter::updateCache() const {
    const auto line_count = m_cursor.getLineCount();

    m_line_cache.clear();
    m_line_cache.resize(line_count);

    if (p_ts_tree == nullptr || p_current_parser == nullptr || p_current_parser->query == nullptr) {
        return;
    }

    const auto root_node = ts_tree_root_node(p_ts_tree);
    ts_query_cursor_exec(p_ts_query_cursor, p_current_parser->query, root_node);

    TSQueryMatch match;
    while (ts_query_cursor_next_match(p_ts_query_cursor, &match)) {
        for (auto i = 0; i < match.capture_count; ++i) {
            const auto capture = match.captures[i];
            const auto node = capture.node;
            const auto start_point = ts_node_start_point(node);
            const auto end_point = ts_node_end_point(node);

            uint32_t capture_name_len;
            const auto capture_name = ts_query_capture_name_for_id(
                p_current_parser->query,
                capture.index,
                &capture_name_len
            );

            auto token_id = TokenId::None;

            const auto name = std::string_view(capture_name, capture_name_len);
            if (name == "keyword") token_id = TokenId::Keyword;
            else if (name == "statement") token_id = TokenId::Statement;
            else if (name == "string") token_id = TokenId::String;
            else if (name == "number") token_id = TokenId::Number;
            else if (name == "comment") token_id = TokenId::Comment;
            else if (name == "preprocessor") token_id = TokenId::Preprocessor;
            else if (name == "type") token_id = TokenId::Type;
            else if (name == "constant") token_id = TokenId::Constant;
            if (token_id == TokenId::None) continue;

            for (auto line = start_point.row; line <= end_point.row && line < line_count; ++line) {
                const auto current_line = m_cursor.getString(line);
                const auto start_col = line == start_point.row ? start_point.column / sizeof(char16_t) : 0;
                const auto end_col = line == end_point.row ? end_point.column / sizeof(char16_t) : current_line.length();

                if (start_col < end_col) {
                    m_line_cache[line].emplace_back(start_col, end_col, token_id);
                }
            }
        }
    }
}

void HighLighter::edit(const BufferEdit &edit) {
    if (p_ts_tree != nullptr) {
        // This just converts and relays the object coming from the cursor
        const auto ts_edit = TSInputEdit {
            .start_byte = edit.start_byte,
            .old_end_byte = edit.old_end_byte,
            .new_end_byte = edit.new_end_byte,
            .start_point = TSPoint(edit.start.line, edit.start.column * sizeof(char16_t)),
            .old_end_point = TSPoint(edit.old_end.line, edit.old_end.column * sizeof(char16_t)),
            .new_end_point = TSPoint(edit.new_end.line, edit.new_end.column * sizeof(char16_t))
        };

        ts_tree_edit(p_ts_tree, &ts_edit);
        m_is_dirty = true;
    }
}

bool HighLighter::isSupported(const std::string_view extension) {
    for (const auto &parser: std::views::values(PARSERS)) {
        if (parser.files_format.contains(extension.data())) {
            return true;
        }
    }

    if (extension == ".txt" || extension == ".TXT") {
        // So the set_hl_command can work with "txt"
        return true;
    }

    return false;
}

void HighLighter::getParserCompletions(const AutoCompleteCallback &callback) {
    // Add a "txt" item, for HighLightId::None
    callback(u"txt");
    for (const auto &parser: std::views::values(PARSERS)) {
        const auto utf8_argument_value = utf8::utf8to16(parser.argument_value);
        callback(utf8_argument_value);
    }
}

TokenId HighLighter::getHighLightAtPosition(const uint32_t line, const uint32_t column) const {
    if (m_high_light == HighLightId::None || line >= m_line_cache.size()) {
        return TokenId::None;
    }

    // Search the spans for the current line
    for (const auto &span : m_line_cache[line]) {
        if (column >= span.start_column && column < span.end_column) {
            return span.token_id;
        }
    }

    return TokenId::None;
}

std::optional<std::u16string_view> HighLighter::readCallback(const uint32_t line, const uint32_t column) const {
    const auto line_count = m_cursor.getLineCount();
    if (line > line_count - 1) {
        // At line_count -1, then there is no more data to process
        return std::nullopt;
    }

    const auto string = m_cursor.getString(line);
    if (line < line_count && column < string.length()) {
        // Not at the very end of the buffer,
        // and not at the end of a line, then there is nothing more to do
        return m_cursor.getString(line).substr(column);
    }

    // Tells the parser there is more
    return u"\n";
}

const char *HighLighter::inputCallback(void *payload, const uint32_t byteIndex, const TSPoint position, uint32_t *bytesRead) {
    // This input callback is working with logical positions, so byteIndex is not needed
    (void) byteIndex;

    // Get the HighLighter instance
    const auto *self = static_cast<HighLighter *>(payload);

    // Multiply and divide column according to char size, we are working with char16_t (2 bytes)
    const auto line = position.row;
    const auto column = position.column / sizeof(char16_t);
    if (const auto &optional_line = self->readCallback(line, column)) {
        // We got some data
        *bytesRead = optional_line->length() * sizeof(char16_t);
        return reinterpret_cast<const char*>(optional_line->data());
    }

    // Tells tree-sitter there is no more data to process at this location
    *bytesRead = 0;
    return nullptr;
}
