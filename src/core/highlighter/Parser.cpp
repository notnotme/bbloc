#include "Parser.h"

#include <cstdint>
#include <stdexcept>


Parser::Parser(const ParserDescriptor &descriptor)
    : m_descriptor(descriptor),
      p_query(nullptr) {

    uint32_t error_offset;
    TSQueryError error_type;
    p_query = ts_query_new(
        m_descriptor.language,
        m_descriptor.query_source.data(),
        m_descriptor.query_source.length(),
        &error_offset,
        &error_type
    );

    if (p_query == nullptr) {
        throw std::runtime_error(
            std::string("Tree-sitter query error for ")
                .append(m_descriptor.name)
                .append(" at offset ")
                .append(std::to_string(error_offset))
        );
    }

    const auto capture_count = ts_query_capture_count(p_query);
    m_capture_token_ids.reserve(capture_count);
    for (uint32_t index = 0; index < capture_count; ++index) {
        uint32_t name_length;
        const auto *name = ts_query_capture_name_for_id(p_query, index, &name_length);
        const auto entry = m_descriptor.capture_tokens.find(std::string(name, name_length));
        m_capture_token_ids.emplace_back(entry != m_descriptor.capture_tokens.end() ? entry->second : TokenId::None);
    }
}

Parser::~Parser() {
    ts_query_delete(p_query);
    p_query = nullptr;
}

const TSLanguage *Parser::getLanguage() const {
    return m_descriptor.language;
}

const TSQuery *Parser::getQuery() const {
    return p_query;
}

TokenId Parser::getTokenId(const uint32_t captureIndex) const {
    return m_capture_token_ids[captureIndex];
}

std::string_view Parser::getName() const {
    return m_descriptor.name;
}

std::string_view Parser::getArgumentValue() const {
    return m_descriptor.argument_value;
}

bool Parser::supportsExtension(const std::string &extension) const {
    return m_descriptor.files_format.contains(extension);
}
