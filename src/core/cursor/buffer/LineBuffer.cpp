#include "LineBuffer.h"

#include <algorithm>


LineBuffer::LineBuffer() {
    // Push one empty line and make it the current line.
    m_line_data.push_back({ .start = 0, .count = 0 });
    m_current_line_index = 0;
}

uint32_t LineBuffer::lineLength(const uint32_t line) const {
    if (line == m_current_line_index) {
        return static_cast<uint32_t>(m_current_line.length());
    }

    return m_line_data[line].count;
}

void LineBuffer::commitCurrentLine() {
    auto &data = m_line_data[m_current_line_index];

    const auto length = static_cast<uint32_t>(m_current_line.length());
    if (length == 0) {
        data.count = 0;
        return;
    }

    // Re-insert the detached characters at the slot reserved for the current line.
    m_buffer.insert(data.start, m_current_line);
    data.count = length;

    // Shift the following lines now that "length" characters were inserted.
    for (auto it = m_line_data.begin() + m_current_line_index + 1; it != m_line_data.end(); ++it) {
        it->start += length;
    }

    m_current_line.clear();
}

std::u16string_view LineBuffer::getString(const uint32_t line) const {
    if (line == m_current_line_index) {
        return std::u16string_view{ m_current_line };
    }

    const auto &line_start = m_buffer.data() + m_line_data[line].start;
    const auto &line_end = line_start + m_line_data[line].count;
    return std::u16string_view{ line_start, line_end };
}

uint32_t LineBuffer::getStringCount() const {
    return static_cast<uint32_t>(m_line_data.size());
}

uint32_t LineBuffer::getByteOffset(const uint32_t line, const uint32_t column) const {
    // m_line_data[line].start already accounts for the current line being absent from
    // the buffer (its detached count is 0), so following offsets stay consistent.
    const auto byte_offset = (m_line_data[line].start + column) * sizeof(char16_t);
    const auto line_ends = line * sizeof(char16_t); // "\n"
    return byte_offset + line_ends;
}

uint32_t LineBuffer::getByteCount(uint32_t lineStart, uint32_t columnStart, uint32_t lineEnd, uint32_t columnEnd) const {
    if (lineStart > lineEnd) {
        // Invert coordinates totally
        std::swap(lineStart, lineEnd);
        std::swap(columnStart, columnEnd);
    } else if (lineStart == lineEnd && columnStart > columnEnd) {
        // Invert column coordinates
        std::swap(columnStart, columnEnd);
    } else if (lineStart == lineEnd && columnStart == columnEnd) {
        return 0;
    }

    // Find the start and end point in the cache, then subtract their offsets. Take in account "\n".
    const auto start_byte_offset = m_line_data[lineStart].start + columnStart;
    const auto end_byte_offset = m_line_data[lineEnd].start + columnEnd;
    const auto line_ends = lineEnd - lineStart; // "\n"
    return (end_byte_offset - start_byte_offset + line_ends) * sizeof(char16_t);
}

BufferEdit LineBuffer::insert(uint32_t line, uint32_t column, const std::u16string_view characters) {
    [[unlikely]] if (characters.empty()) {
        return {0,0,0, {line,column}, {line,column}, {line,column}};
    }

    // Fast path: single-line insert (no "\n") into the current line only touches m_current_line.
    if (line == m_current_line_index && characters.find(U'\n') == std::u16string_view::npos) {
        auto edit = BufferEdit();
        edit.start_byte = getByteOffset(line, column);
        edit.old_end_byte = edit.start_byte;
        edit.new_end_byte = static_cast<uint32_t>(edit.start_byte + characters.length() * sizeof(char16_t));
        edit.start.line = line;
        edit.start.column = column;
        edit.old_end.line = line;
        edit.old_end.column = column;
        edit.new_end.line = line;
        edit.new_end.column = static_cast<uint32_t>(column + characters.length());

        m_current_line.insert(column, characters);
        return edit;
    }

    // Slow path: the big buffer is involved. Fold everything back so the buffer is consistent.
    commitCurrentLine();

    auto edit = BufferEdit();
    edit.start_byte = getByteOffset(line, column);
    edit.old_end_byte = edit.start_byte;
    edit.new_end_byte = static_cast<uint32_t>(edit.start_byte + characters.length() * sizeof(char16_t));
    edit.start.line = line;
    edit.start.column = column;
    edit.old_end.line = line;
    edit.old_end.column = column;

    // Start at the said line
    auto line_data = m_line_data.begin() + line;

    // Calculate the remainder length then resize current line_data
    const auto remainder_length = line_data->count - column;
    line_data->count = column;

    auto inserted_total = 0u;
    auto segment_start = 0;
    auto insert_offset = line_data->start + column;

    const auto characters_count = characters.length();
    for (auto i = 0; i < characters_count; ++i) {
        if (characters[i] == U'\n') {
            // found delimiter
            const auto segment_length = static_cast<uint32_t>(i - segment_start);
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

    const auto final_segment_len = static_cast<uint32_t>(characters.length() - segment_start);
    if (final_segment_len > 0) {
        // Insert the final segment (after last \n or whole string if no \n)
        const auto segment_view = characters.substr(segment_start, final_segment_len);
        m_buffer.insert(insert_offset, segment_view);

        // Increments the counters
        line_data->count += final_segment_len;
        inserted_total += final_segment_len;
    }

    // Place the cursor at the right column position, then re-append the remainder count the final line
    column = line_data->count;
    if (remainder_length > 0) {
        line_data->count += remainder_length;
    }

    // Insert operations can increment the line and column variable.
    // We know the last position now, we can fill the last bit of the edit struct.
    edit.new_end.line = line;
    edit.new_end.column = column;

    // The line where the edit ended as the new current line.
    m_current_line_index = line;

    auto &data = m_line_data[line];
    const auto offset = data.start;
    const auto length = data.count;

    m_current_line.assign(m_buffer, offset, length);
    m_buffer.erase(offset, length);
    data.count = 0;

    // Shift all the following line offsets
    for (auto it = m_line_data.begin() + line + 1; it != m_line_data.end(); ++it) {
        it->start += inserted_total - length;
    }

    return edit;
}

BufferEdit LineBuffer::erase(uint32_t line, uint32_t column, uint32_t lineEnd, uint32_t columnEnd) {
    if (line > lineEnd) {
        // Invert coordinates totally
        std::swap(line, lineEnd);
        std::swap(column, columnEnd);
    } else if (line == lineEnd && column > columnEnd) {
        // Invert column coordinates
        std::swap(column, columnEnd);
    } else if (line == lineEnd && column == columnEnd) {
        return {0,0,0, {0,0}, {0,0}, {0,0}};
    }

    // Fast path: erase within the current line only touches m_current_line.
    if (line == lineEnd && line == m_current_line_index) {
        // Find start and end byte
        const auto start_byte  = getByteOffset(line, column);
        const auto end_byte = getByteOffset(lineEnd, columnEnd);

        // We need to fill a BufferEdit struct
        auto edit = BufferEdit();
        edit.start_byte = start_byte;
        edit.old_end_byte = end_byte;
        edit.new_end_byte = edit.start_byte;

        // Start at line, column, and the old end point is at lineEnd, columnEnd
        edit.start.line = line;
        edit.start.column = column;
        edit.old_end.line = lineEnd;
        edit.old_end.column = columnEnd;
        edit.new_end.line = line;
        edit.new_end.column = column;

        m_current_line.erase(column, columnEnd - column);
        return edit;
    }

    // Slow path: the big buffer is involved. Fold everything back so the buffer is consistent.
    commitCurrentLine();

    // Find start and end byte
    const auto start_byte = getByteOffset(line, column);
    const auto end_byte = getByteOffset(lineEnd, columnEnd);

    // We need to fill a BufferEdit struct
    auto edit = BufferEdit();
    edit.start_byte = start_byte;
    edit.old_end_byte = end_byte;
    edit.new_end_byte = start_byte;

    // Start at line, column, and the old end point is at lineEnd, columnEnd
    edit.start.line = line;
    edit.start.column = column;
    edit.old_end.line = lineEnd;
    edit.old_end.column = columnEnd;
    edit.new_end.line = line;
    edit.new_end.column = column;

    // Get the iterator from first nd last line to erase in the range, and the column offset (in character count)
    const auto start_line = m_line_data.begin() + line;
    const auto end_line = m_line_data.begin() + lineEnd;
    const auto start_offset = start_line->start + column;
    const auto end_offset = end_line->start + columnEnd;

    // Compute erase length and remove from the buffer
    const auto erase_length = end_offset - start_offset;
    m_buffer.erase(start_offset, erase_length);

    if (line == lineEnd) {
        // Same line, erase the necessary part from the line data only
        start_line->count -= erase_length;
    } else {
        // Resize the first line_data in the range
        start_line->count = column + (end_line->count - columnEnd);
        // Remove all intermediate line_data, including end_line
        m_line_data.erase(m_line_data.begin() + line + 1, m_line_data.begin() + lineEnd + 1);
    }

    // The line where the edit ended as the new current line.
    m_current_line_index = line;

    auto &data = m_line_data[line];
    const auto offset = data.start;
    const auto length = data.count;

    m_current_line.assign(m_buffer, offset, length);
    m_buffer.erase(offset, length);
    data.count = 0;

    // Shift all the following line offsets
    for (auto it = m_line_data.begin() + line + 1; it != m_line_data.end(); ++it) {
        it->start -= length + erase_length;
    }

    return edit;
}

BufferEdit LineBuffer::clear() {
    // Make sure the current line is folded back so the sizes below are exact.
    commitCurrentLine();

    const auto &last_line_data = m_line_data.back();
    const auto line_count = m_line_data.size();
    const auto buffer_size = getByteOffset(line_count - 1, last_line_data.count);

    m_buffer.clear();
    m_current_line.clear();
    m_line_data.clear();
    m_line_data.push_back({ .start = 0, .count = 0 });
    m_current_line_index = 0;

    return {
        .start_byte = buffer_size,
        .old_end_byte = buffer_size,
        .new_end_byte = 0,
        .start = {
            .line = static_cast<uint32_t>(line_count),
            .column = static_cast<uint32_t>(last_line_data.count)
        },
        .old_end = {
            .line = static_cast<uint32_t>(line_count),
            .column = static_cast<uint32_t>(last_line_data.count)
        },
        .new_end = {
            .line = 0,
            .column = 0
        }
    };
}