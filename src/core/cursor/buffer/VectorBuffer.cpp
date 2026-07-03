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

    // Start to count, ! -> "\n" -> +1 length
    // The first line counts from columnStart and contains a line end, the last line
    // counts up to columnEnd without a line end, every other line counts entirely.
    auto unit_count = m_lines[lineStart].length() - columnStart + 1;
    for (auto current_line = lineStart + 1; current_line < lineEnd; ++current_line) {
        unit_count += m_lines[current_line].length() + 1;
    }
    unit_count += columnEnd;
    return static_cast<uint32_t>(unit_count * sizeof(char16_t));
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
        // Empty range, describe the untouched position
        const auto byte_offset = getByteOffset(line, column);
        return {byte_offset, byte_offset, byte_offset, {line, column}, {line, column}, {line, column}};
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
        // Truncate the first line, append the remainder of the last line, then drop the lines in between
        auto &first_line = m_lines[line];
        first_line.resize(column);
        first_line.append(m_lines[lineEnd], columnEnd);
        m_lines.erase(m_lines.begin() + line + 1, m_lines.begin() + lineEnd + 1);
    }

    // We can finish filling the BufferEdit struct
    edit.new_end.line = line;
    edit.new_end.column = column;
    return edit;
}

BufferEdit VectorBuffer::clear() {
    // Clear everything, push one empty line, keep some number in memory before so we can fill the BufferEdit struct.
    const auto last_line = static_cast<uint32_t>(m_lines.size()) - 1;
    const auto last_column = static_cast<uint32_t>(m_lines.back().length());
    const auto buffer_size = getByteOffset(last_line, last_column);

    m_lines.clear();
    m_lines.emplace_back();
    return {
        .start_byte = 0,
        .old_end_byte = buffer_size,
        .new_end_byte = 0,
        .start = {
            .line = 0,
            .column = 0
        },
        .old_end = {
            .line = last_line,
            .column = last_column
        },
        .new_end = {
            .line = 0,
            .column = 0
        }
    };
}
