#include "StringBuffer.h"


StringBuffer::StringBuffer() {
    // Push one empty line
    m_line_data.push_back({ .start = 0, .count = 0 });
}

std::u16string_view StringBuffer::getString(const int32_t line) const {
    const auto& line_start = m_buffer.data() + m_line_data[line].start;
    const auto& line_end = line_start + m_line_data[line].count;
    return std::u16string_view {line_start, line_end};
}

int32_t StringBuffer::getStringCount() const {
    return static_cast<int32_t>(m_line_data.size());
}

void StringBuffer::insert(int32_t& line, int32_t& column, const std::u16string_view characters) {
    constexpr auto delimiter = U'\n';

    // Start at the said line
    auto line_data = m_line_data.begin() + line;

    // Compute and temporarily extract the reminder of the line, adjust count for the current line before inserting
    const auto characters_count = characters.length();
    const auto reminder_length = line_data->count - column;
    line_data->count = column;

    auto inserted_total = 0;
    auto segment_start = 0;
    auto insert_offset = line_data->start + column;
    for (auto i = 0; i < characters_count; ++i) {
        if (characters[i] == delimiter) {
            // found delimiter
            const auto segment_length = i - segment_start;
            if (segment_length > 0) {
                // Extract the string sequence and appends to the buffer
                const auto segment_view = characters.substr(segment_start, segment_length);
                m_buffer.insert(insert_offset, segment_view);
                // Increments the counters
                line_data->count += segment_length;
                insert_offset += segment_length;
                inserted_total += segment_length;
            }

            // Move to the next line
            ++line;
            line_data = m_line_data.insert(m_line_data.begin() + line, { .start = insert_offset, .count = 0 });
            segment_start = i + 1;
        }
    }

    // Insert final segment (after last \n or whole string if no \n)
    const auto final_segment_len = static_cast<int32_t>(characters.length() - segment_start);
    if (final_segment_len > 0) {
        const auto segment_view = characters.substr(segment_start, final_segment_len);
        m_buffer.insert(insert_offset, segment_view);
        line_data->count += final_segment_len;
        inserted_total += final_segment_len;
    }

    // Place the cursor at the right column position, then re-append the reminder count the final line
    column = line_data->count;
    if (reminder_length > 0) {
        line_data->count += reminder_length;
    }

    // Shift all the following line offsets
    for (auto it = m_line_data.begin() + line + 1; it != m_line_data.end(); ++it) {
        it->start += inserted_total;
    }
}

void StringBuffer::erase(int32_t& line, int32_t& column, int32_t lineEnd, int32_t columnEnd) {
    if (line > lineEnd) {
        // Invert coordinates
        std::swap(line, lineEnd);
        std::swap(column, columnEnd);
    } else if (line == lineEnd && column > columnEnd) {
        // Invert coordinates
        std::swap(column, columnEnd);
    }

    const auto start_line = m_line_data.begin() + line;
    const auto end_line = m_line_data.begin() + lineEnd;
    const auto start_offset = start_line->start + column;
    const auto end_offset = end_line->start + columnEnd;

    // Compute erase length and remove from the buffer
    const auto erase_length = end_offset - start_offset;
    m_buffer.erase(start_offset, erase_length);

    if (line == lineEnd) {
        // Same line, erase the necessary part
        start_line->count -= erase_length;
    } else {
        // Truncate the first line and last line with their reminders
        start_line->count = column + (end_line->count - columnEnd);
        // Remove all intermediate lines, including end_line
        m_line_data.erase(m_line_data.begin() + line + 1, m_line_data.begin() + lineEnd + 1);
    }

    // Shift all the following line offsets
    for (auto it = m_line_data.begin() + line + 1; it != m_line_data.end(); ++it) {
        it->start -= erase_length;
    }
}

void StringBuffer::clear() {
    // Clear everything, push one empty line
    m_buffer.clear();
    m_line_data.clear();
    m_line_data.emplace_back();
}
