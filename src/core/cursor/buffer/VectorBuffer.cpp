#include "VectorBuffer.h"

#include <stdexcept>


VectorBuffer::VectorBuffer() {
    // Push one empty line
    m_lines.emplace_back();
}

std::u16string_view VectorBuffer::getString(const int32_t line) const {
    return std::u16string_view { m_lines[line] };
}

int32_t VectorBuffer::getStringCount() const {
    return static_cast<int32_t>(m_lines.size());
}

void VectorBuffer::insert(int32_t& line, int32_t& column, const std::u16string_view characters) {
    constexpr auto delimiter = U'\n';

    // Start at the said line
    auto current_line = m_lines.begin() + line;
    // Compute and temporarily extract the reminder of the line, adjust the line size accordingly before inserting
    const auto is_single_line_insert = characters.find(U'\n') == std::u16string_view::npos;
    const auto characters_count = static_cast<int32_t>(characters.length());

    if (is_single_line_insert) {
        current_line->insert(column, characters);
        column += characters_count;
    } else {
        const auto reminder = current_line->substr(column);
        current_line->resize(column);

        auto segment_start = 0;
        for (auto i = 0; i < characters_count; ++i) {
            if (characters[i] == delimiter) {
                // Insert chunk before newline
                if (i > segment_start) {
                    const auto segment_view = characters.substr(segment_start, i - segment_start);
                    current_line->append(segment_view);
                }

                // New line slot
                ++line;
                current_line = m_lines.emplace(m_lines.begin() + line);
                segment_start = i + 1;
            }
        }

        // Insert remaining part after last newline
        if (segment_start < characters_count) {
            const auto segment_view = characters.substr(segment_start);
            current_line->append(segment_view);
        }

        // Move the cursor at the right column position, then append the reminder content to the final line
        column = static_cast<int32_t>(current_line->length());
        current_line->append(reminder);
    }
}

void VectorBuffer::erase(int32_t& line, int32_t& column, int32_t lineEnd, int32_t columnEnd) {
    if (line > lineEnd) {
        // Invert coordinates
        std::swap(line, lineEnd);
        std::swap(column, columnEnd);
    } else if (line == lineEnd && column > columnEnd) {
        // Invert coordinates
        std::swap(column, columnEnd);
    }

    if (line == lineEnd) {
        // Same line, erase the necessary part
        const auto string = m_lines.begin() + lineEnd;
        string->erase(column, columnEnd - column);
    } else {
        // Keep the reminder of the first and last line
        const auto first_part_to_append = (m_lines.begin() + line)->substr(0, column);
        const auto last_part_to_append = (m_lines.begin() + lineEnd)->substr(columnEnd);
        // Remove all lines
        m_lines.erase(m_lines.begin() + line, m_lines.begin() + lineEnd + 1);
        // Add the reminder of the first and last line that we cut
        m_lines.emplace(m_lines.begin() + line, first_part_to_append + last_part_to_append);
    }
}

void VectorBuffer::clear() {
    // Clear everything, push one empty line
    m_lines.clear();
    m_lines.emplace_back();
}
