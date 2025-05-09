#include "VectorBuffer.h"

#include <stdexcept>


VectorBuffer::VectorBuffer() {
    // Push one empty line
    m_lines.emplace_back();
}

std::u16string_view VectorBuffer::getString(const uint32_t line) const {
    return std::u16string_view { m_lines[line] };
}

uint32_t VectorBuffer::getStringCount() const {
    return m_lines.size();
}

uint32_t VectorBuffer::getByteOffset(const uint32_t line, const uint32_t column) const {
    // Count column position byte offset, then go through all lines
    // to compute their size in byte. ! -> "\n" -> +1 length
    auto start_byte = column * sizeof(char16_t);
    for (auto current_line = 0; current_line < line; ++current_line) {
        const auto &string = m_lines[current_line];
        start_byte += (string.length() + 1) * sizeof(char16_t);
    }
    return start_byte;
}

uint32_t VectorBuffer::getByteCount(uint32_t lineStart, uint32_t columnStart, uint32_t lineEnd, uint32_t columnEnd) const {
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

    if (lineStart == lineEnd) {
        // Simple enough, no line ends
        return (columnEnd - columnStart) * sizeof(char16_t);
    }

    auto byte_count = 0u;
    for (auto current_line = lineStart; current_line <= lineEnd; ++current_line) {
        // Start to count, ! -> "\n" -> +1 length
        const auto &string = m_lines[current_line];
        if (current_line == lineStart) {
            // The first line always contains the line end, and we add only from columnStart
            const auto &remainder = string.substr(columnStart);
            const auto remainder_length = remainder.length();
            byte_count += (remainder_length + 1) * sizeof(char16_t);
        } else if (current_line == lineEnd) {
            // The last line does not contains any line end, we add only from 0 to columnEnd
            const auto &noun = string.substr(0, columnEnd);
            const auto noun_length = noun.length();
            byte_count += noun_length * sizeof(char16_t);
        } else {
            // Every other line adds all the string plus the line end.
            const auto string_length = string.length();
            byte_count += (string_length + 1) * sizeof(char16_t);
        }
    }
    return byte_count;
}

BufferEdit VectorBuffer::insert(uint32_t line, uint32_t column, const std::u16string_view characters) {
    [[unlikely]] if (characters.empty()) {
        // This will likely never happen?
        return {0,0,0, {line, column}, {line, column}, {line, column}};
    }

    // As we insert into the buffer, we need to fill a BufferEdit
    auto edit = BufferEdit();

    // Start byte and old end byte are the same
    edit.start_byte = getByteOffset(line, column);
    edit.old_end_byte = edit.start_byte;

    // New end byte is characters size in byte.
    edit.new_end_byte = edit.start_byte + characters.length() * sizeof(char16_t);

    // Start and old end is always the same.
    edit.start.line = line;
    edit.start.column = column;
    edit.old_end.line = line;
    edit.old_end.column = column;

    // Start at the said line
    auto current_line = m_lines.begin() + line;

    // Quick check to see if we only insert in the same line
    const auto is_single_line_insert = characters.find(U'\n') == std::u16string_view::npos;
    const auto characters_count = static_cast<int32_t>(characters.length());
    if (is_single_line_insert) {
        current_line->insert(column, characters);
        column += characters_count;
    } else {
        // Multi line insert, keep the remainder of the first line
        const auto &remainder = current_line->substr(column);
        current_line->resize(column);

        auto segment_start = 0;
        for (auto i = 0; i < characters_count; ++i) {
            // Adds every line to the buffer
            if (characters[i] == U'\n') {
                if (i > segment_start) {
                    const auto segment_view = characters.substr(segment_start, i - segment_start);
                    current_line->append(segment_view);
                }

                // Increment line and create a new line slot
                ++line;
                current_line = m_lines.emplace(m_lines.begin() + line);
                segment_start = i + 1;
            }
        }

        // Insert the remaining part after last new line
        if (segment_start < characters_count) {
            const auto segment_view = characters.substr(segment_start);
            current_line->append(segment_view);
        }

        // Move the cursor at the right column position, then append the remainder content to the final line
        column = static_cast<int32_t>(current_line->length());
        current_line->append(remainder);
    }

    // We can now finish filling the BufferEdit since line and column probably changed values.
    edit.new_end.line = line;
    edit.new_end.column = column;
    return edit;
}

BufferEdit VectorBuffer::erase(uint32_t line, uint32_t column, uint32_t lineEnd, uint32_t columnEnd) {
    if (line > lineEnd) {
        // Invert coordinates totally
        std::swap(line, lineEnd);
        std::swap(column, columnEnd);
    } else if (line == lineEnd && column > columnEnd) {
        // Invert column coordinates
        std::swap(column, columnEnd);
    } else if (line == lineEnd && column == columnEnd) {
        return {0,0,0, {line, column}, {line, column}, {line, column}};
    }

    // Find the start byte and the byte count of the range to delete.
    const auto start_byte  = getByteOffset(line, column);
    const auto byte_count = getByteCount(line, column, lineEnd, columnEnd);

    // We need to fill a BufferEdit struct
    auto edit = BufferEdit();
    edit.start_byte = start_byte;

    // The old end byte is the fartest point in the range
    edit.old_end_byte = edit.start_byte + byte_count;

    // The new end byte is the same as the start
    edit.new_end_byte = start_byte;

    // We start at line, column, and the old end point is at lineEnd, columnEnd
    edit.start.line = line;
    edit.start.column = column;
    edit.old_end.line = lineEnd;
    edit.old_end.column = columnEnd;

    if (line == lineEnd) {
        // Same line, erase the necessary part
        const auto string = m_lines.begin() + lineEnd;
        string->erase(column, columnEnd - column);
    } else {
        // Keep the remainder of the first and last line
        const auto &first_part_to_append = (m_lines.begin() + line)->substr(0, column);
        const auto &last_part_to_append = (m_lines.begin() + lineEnd)->substr(columnEnd);

        // Remove all lines in between start / end.
        m_lines.erase(m_lines.begin() + line, m_lines.begin() + lineEnd + 1);

        // Add the remainder of the first and last line that we cut
        m_lines.emplace(m_lines.begin() + line, first_part_to_append + last_part_to_append);
    }

    // We can finish filling the BufferEdit struct
    edit.new_end.line = line;
    edit.new_end.column = column;
    return edit;
}

BufferEdit VectorBuffer::clear() {
    // Clear everything, push one empty line, keep some number in memory before so we can fill the BufferEdit struct.
    const auto &last_string = m_lines.back();
    const auto line_count = m_lines.size();
    const auto last_string_length = last_string.length();
    const auto buffer_size = getByteOffset(line_count - 1, last_string_length);

    m_lines.clear();
    m_lines.emplace_back();
    return {
        .start_byte = buffer_size,
        .old_end_byte = buffer_size,
        .new_end_byte = 0,
        .start = {
            .line = static_cast<uint32_t>(line_count),
            .column = static_cast<uint32_t>(last_string_length)
        },
        .old_end = {
            .line = static_cast<uint32_t>(line_count),
            .column = static_cast<uint32_t>(last_string_length)
        },
        .new_end = {
            .line = 0,
            .column = 0
        }
    };
}
