#include "HighLighter.h"

#include <ranges>
#include <utf8.h>

#include "ParserCatalog.h"


HighLighter::HighLighter(Cursor &cursor)
    : m_cursor(cursor),
      m_parsers(ParserCatalog::createParsers()),
      p_current_parser(nullptr),
      p_ts_parser(ts_parser_new()),
      p_ts_tree(nullptr),
      p_ts_query_cursor(ts_query_cursor_new()),
      m_input(this, inputCallback, TSInputEncodingUTF16LE),
      m_high_light(HighLightId::None),
      m_is_dirty(false) {}

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
}

void HighLighter::setMode(const HighLightId highLight) {
    m_high_light = highLight;
    m_is_dirty = true;

    if (highLight == HighLightId::None) {
        // There is no need to keep the old mode
        p_current_parser = nullptr;
    } else {
        // Set new mode
        const auto &parser = m_parsers.at(highLight);
        if (! ts_parser_set_language(p_ts_parser, parser.getLanguage())) {
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
    setMode(ParserCatalog::findModeByExtension(extension));
}

std::string_view HighLighter::getModeString() const {
    if (m_high_light == HighLightId::None) {
        // None is TEXT
        return "TEXT";
    }

    return p_current_parser->getName();
}

void HighLighter::parse() {
    if (p_current_parser != nullptr && m_is_dirty) {
        // Reuse the old tree to parse and create a new one; if p_ts_tree is nullptr, parse from scratch
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

    if (p_ts_tree == nullptr || p_current_parser == nullptr) {
        return;
    }

    const auto root_node = ts_tree_root_node(p_ts_tree);
    ts_query_cursor_exec(p_ts_query_cursor, p_current_parser->getQuery(), root_node);

    TSQueryMatch match;
    while (ts_query_cursor_next_match(p_ts_query_cursor, &match)) {
        for (auto i = 0; i < match.capture_count; ++i) {
            const auto capture = match.captures[i];
            const auto node = capture.node;
            const auto start_point = ts_node_start_point(node);
            const auto end_point = ts_node_end_point(node);

            const auto token_id = p_current_parser->getTokenId(capture.index);
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
    if (ParserCatalog::findModeByExtension(extension) != HighLightId::None) {
        return true;
    }

    // So the set_hl_command can work with "txt"
    return extension == ".txt" || extension == ".TXT";
}

void HighLighter::getParserCompletions(const AutoCompleteCallback &callback) {
    // Add a "txt" item, for HighLightId::None
    callback(u"txt");
    for (const auto &descriptor: std::views::values(ParserCatalog::getDescriptors())) {
        const auto utf16_argument_value = utf8::utf8to16(descriptor.argument_value);
        callback(utf16_argument_value);
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
    if (line >= line_count) {
        return std::nullopt;
    }

    const auto string = m_cursor.getString(line);
    if (column < string.length()) {
        return string.substr(column);
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
