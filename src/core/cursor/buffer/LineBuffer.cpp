/*
* Copyright (C) 2026 Romain Graillot
 *
 * This file is part of bbloc.
 *
 * bbloc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bbloc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "LineBuffer.h"

#include <algorithm>
#include <tuple>


LineBuffer::LineBuffer() {
    // Push one empty line and make it the current line.
    m_line_data.push_back(LineData{.start = 0, .count = 0});
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

uint32_t LineBuffer::getLongestLineLength(const uint32_t tabWeight) const {
    return m_longest_line.getLongestLineLength(tabWeight);
}

uint32_t LineBuffer::getLineTabCount(const uint32_t line) const {
    return m_longest_line.getLineTabCount(line);
}

uint32_t LineBuffer::detachedLengthBefore(const uint32_t line) const {
    return line > m_current_line_index ? static_cast<uint32_t>(m_current_line.length()) : 0;
}

uint32_t LineBuffer::getByteOffset(const uint32_t line, const uint32_t column) const {
    // m_line_data[line].start excludes the detached current line's characters,
    // so lines after it must be shifted by its length to stay consistent.
    // The sizeof products widen to size_t; byte offsets are 32-bit (tree-sitter's own width).
    const auto byte_offset = static_cast<uint32_t>((m_line_data[line].start + column + detachedLengthBefore(line)) * sizeof(char16_t));
    const auto line_ends = static_cast<uint32_t>(line * sizeof(char16_t)); // "\n"
    return byte_offset + line_ends;
}

uint32_t LineBuffer::getByteCount(uint32_t lineStart, uint32_t columnStart, uint32_t lineEnd, uint32_t columnEnd) const {
    if (std::tie(lineStart, columnStart) > std::tie(lineEnd, columnEnd)) {
        // Invert coordinates (swapping equal lines is a no-op)
        std::swap(lineStart, lineEnd);
        std::swap(columnStart, columnEnd);
    } else if (lineStart == lineEnd && columnStart == columnEnd) {
        return 0;
    }

    // Find the start and end point in the line metadata, then subtract their offsets. Take in account "\n".
    const auto start_byte_offset = m_line_data[lineStart].start + columnStart + detachedLengthBefore(lineStart);
    const auto end_byte_offset = m_line_data[lineEnd].start + columnEnd + detachedLengthBefore(lineEnd);
    const auto line_ends = lineEnd - lineStart; // "\n"
    return static_cast<uint32_t>((end_byte_offset - start_byte_offset + line_ends) * sizeof(char16_t));
}

BufferEdit LineBuffer::insert(uint32_t line, uint32_t column, const std::u16string_view characters) {
    [[unlikely]] if (characters.empty()) {
        // Nothing inserted, describe the untouched position: byte offsets left at 0 would disagree
        // with the points and make the incremental re-parse dirty the whole start of the tree.
        const auto byte_offset = getByteOffset(line, column);
        return BufferEdit{
            .start_byte = byte_offset,
            .old_end_byte = byte_offset,
            .new_end_byte = byte_offset,
            .start = {.line = line, .column = column},
            .old_end = {.line = line, .column = column},
            .new_end = {.line = line, .column = column}
        };
    }

    // Fast path: single-line insert (no "\n") into the current line only touches m_current_line.
    if (line == m_current_line_index && characters.find(U'\n') == std::u16string_view::npos) {
        const auto start_byte = getByteOffset(line, column);
        const auto edit = BufferEdit{
            .start_byte = start_byte,
            .old_end_byte = start_byte,
            .new_end_byte = static_cast<uint32_t>(start_byte + characters.length() * sizeof(char16_t)),
            .start = {.line = line, .column = column},
            .old_end = {.line = line, .column = column},
            .new_end = {.line = line, .column = static_cast<uint32_t>(column + characters.length())}
        };

        m_current_line.insert(column, characters);
        m_longest_line.onEdit(*this, edit);
        return edit;
    }

    // Fast path: a bare newline on the current line is only a split of m_current_line. The slow
    // path would commit the whole line back and pull the very same tail out again, paying two
    // buffer moves and three sweeps of m_line_data for a split that costs one of each.
    if (line == m_current_line_index && characters == u"\n") {
        const auto start_byte = getByteOffset(line, column);
        const auto edit = BufferEdit{
            .start_byte = start_byte,
            .old_end_byte = start_byte,
            .new_end_byte = static_cast<uint32_t>(start_byte + sizeof(char16_t)),
            .start = {.line = line, .column = column},
            .old_end = {.line = line, .column = column},
            .new_end = {.line = line + 1, .column = 0}
        };

        // Commit the head only: it stays on this line, at the slot the current line already owns.
        const auto line_start = m_line_data[line].start;
        m_buffer.insert(line_start, m_current_line, 0, column);
        m_line_data[line].count = column;

        // Open the slot of the line the split creates, right where the head ends.
        m_line_data.insert(m_line_data.begin() + line + 1, LineData{.start = line_start + column, .count = 0});

        // Only the head reached the buffer, so the lines below move by exactly its length.
        for (auto it = m_line_data.begin() + line + 2; it != m_line_data.end(); ++it) {
            it->start += column;
        }

        // The tail is already detached: dropping the head in place makes it the new current line.
        m_current_line.erase(0, column);
        m_current_line_index = line + 1;

        m_longest_line.onEdit(*this, edit);
        return edit;
    }

    // Slow path: the big buffer is involved. Fold everything back so the buffer is consistent.
    commitCurrentLine();

    // The edit's new_end is only known further down, once the trailing segment has been measured.
    const auto start_byte = getByteOffset(line, column);
    auto edit = BufferEdit{
        .start_byte = start_byte,
        .old_end_byte = start_byte,
        .new_end_byte = static_cast<uint32_t>(start_byte + characters.length() * sizeof(char16_t)),
        .start = {.line = line, .column = column},
        .old_end = {.line = line, .column = column},
        .new_end = {}
    };

    // The segments land back to back at the same buffer offset, so the whole insertion is one splice:
    // count the newlines up front, then move the buffer and the line metadata exactly once each.
    const auto newline_count = static_cast<uint32_t>(std::count(characters.begin(), characters.end(), u'\n'));
    const auto end_line = line + newline_count;

    // Read the touched line's geometry before the metadata is reshaped below.
    const auto remainder_length = m_line_data[line].count - column;
    const auto insert_offset = m_line_data[line].start + column;

    // Make room for all the new lines in one shift of the metadata.
    if (newline_count > 0) {
        m_line_data.insert(m_line_data.begin() + line + 1, newline_count, LineData{});
    }

    // The buffer holds the text without its line ends; a newline-free insert needs no flattening copy.
    auto flattened = std::u16string{};
    auto flattened_view = characters;
    size_t segment_start = 0;

    if (newline_count > 0) {
        flattened.reserve(characters.length() - newline_count);

        // Single pass over the segments: each newline closes the line it ends and opens the next one,
        // whose start is the offset the already flattened characters push it to.
        auto current_line = line;
        for (size_t i = 0; i < characters.length(); ++i) {
            if (characters[i] != u'\n') {
                continue;
            }

            flattened.append(characters, segment_start, i - segment_start);
            m_line_data[current_line].count = (current_line == line ? column : 0) + static_cast<uint32_t>(i - segment_start);

            ++current_line;
            m_line_data[current_line].start = insert_offset + static_cast<uint32_t>(flattened.length());
            segment_start = i + 1;
        }

        flattened.append(characters, segment_start, characters.length() - segment_start);
        flattened_view = flattened;
    }

    // The trailing segment lands on the line where the edit ends, which also takes back the remainder.
    auto &end_data = m_line_data[end_line];
    end_data.count = (end_line == line ? column : 0) + static_cast<uint32_t>(characters.length() - segment_start);

    // The insertion ends one line further down per newline, at the length of the trailing segment.
    // We know the last position now, we can fill the last bit of the edit struct.
    edit.new_end.line = end_line;
    edit.new_end.column = end_data.count;

    end_data.count += remainder_length;

    // Splice the flattened characters into the buffer in a single move.
    m_buffer.insert(insert_offset, flattened_view);

    // The line where the edit ended as the new current line.
    m_current_line_index = end_line;

    const auto offset = end_data.start;
    const auto length = end_data.count;

    m_current_line.assign(m_buffer, offset, length);
    m_buffer.erase(offset, length);
    end_data.count = 0;

    // Shift all the following line offsets
    const auto inserted_total = static_cast<uint32_t>(flattened_view.length());
    for (auto it = m_line_data.begin() + end_line + 1; it != m_line_data.end(); ++it) {
        it->start += inserted_total - length;
    }

    m_longest_line.onEdit(*this, edit);
    return edit;
}

BufferEdit LineBuffer::erase(uint32_t line, uint32_t column, uint32_t lineEnd, uint32_t columnEnd) {
    if (std::tie(line, column) > std::tie(lineEnd, columnEnd)) {
        // Invert coordinates (swapping equal lines is a no-op)
        std::swap(line, lineEnd);
        std::swap(column, columnEnd);
    } else if (line == lineEnd && column == columnEnd) {
        // Empty range, describe the untouched position
        const auto byte_offset = getByteOffset(line, column);
        return BufferEdit{
            .start_byte = byte_offset,
            .old_end_byte = byte_offset,
            .new_end_byte = byte_offset,
            .start = {.line = line, .column = column},
            .old_end = {.line = line, .column = column},
            .new_end = {.line = line, .column = column}
        };
    }

    // Fast path: erase within the current line only touches m_current_line.
    if (line == lineEnd && line == m_current_line_index) {
        // Find start and end byte
        const auto start_byte  = getByteOffset(line, column);
        const auto end_byte = getByteOffset(lineEnd, columnEnd);

        // We need to fill a BufferEdit struct: it starts at line, column, and the old
        // end point is at lineEnd, columnEnd.
        const auto edit = BufferEdit{
            .start_byte = start_byte,
            .old_end_byte = end_byte,
            .new_end_byte = start_byte,
            .start = {.line = line, .column = column},
            .old_end = {.line = lineEnd, .column = columnEnd},
            .new_end = {.line = line, .column = column}
        };

        m_current_line.erase(column, columnEnd - column);
        m_longest_line.onEdit(*this, edit);
        return edit;
    }

    // Fast path: the mirror of the bare-newline insert. Joining a line with the one right below it
    // only merges m_current_line with a single neighbouring slot, as long as the current line is one
    // of the two: backspace at column 0 detaches the lower line, delete at the end the upper one.
    if (lineEnd == line + 1 && columnEnd == 0 && (m_current_line_index == line || m_current_line_index == line + 1)) {
        // Both offsets are read before anything moves, where the slow path reads them after folding
        // the current line back. The two agree: committing does not move the current line's own
        // start, and detachedLengthBefore already accounts for it on the line below.
        const auto start_byte = getByteOffset(line, column);
        const auto edit = BufferEdit{
            .start_byte = start_byte,
            .old_end_byte = getByteOffset(lineEnd, columnEnd),
            .new_end_byte = start_byte,
            .start = {.line = line, .column = column},
            .old_end = {.line = lineEnd, .column = columnEnd},
            .new_end = {.line = line, .column = column}
        };

        if (m_current_line_index == line + 1) {
            // The lower line is detached: pull the upper line's head in front of it, then drop the
            // upper line whole (head and erased tail alike) out of the buffer.
            const auto upper_start = m_line_data[line].start;
            const auto upper_length = m_line_data[line].count;
            m_current_line.insert(0, m_buffer, upper_start, column);
            m_buffer.erase(upper_start, upper_length);
            m_line_data[line].count = 0;
            m_line_data.erase(m_line_data.begin() + line + 1);

            for (auto it = m_line_data.begin() + line + 1; it != m_line_data.end(); ++it) {
                it->start -= upper_length;
            }
        } else {
            // The upper line is detached: cut its tail off and append the lower line to it, then
            // drop the lower line out of the buffer.
            const auto lower_start = m_line_data[line + 1].start;
            const auto lower_length = m_line_data[line + 1].count;
            m_current_line.erase(column);
            m_current_line.append(m_buffer, lower_start, lower_length);
            m_buffer.erase(lower_start, lower_length);
            m_line_data.erase(m_line_data.begin() + line + 1);

            for (auto it = m_line_data.begin() + line + 1; it != m_line_data.end(); ++it) {
                it->start -= lower_length;
            }
        }

        // The joined line is the one the edit ends on, and it is the detached one either way.
        m_current_line_index = line;

        m_longest_line.onEdit(*this, edit);
        return edit;
    }

    // Slow path: the big buffer is involved. Fold everything back so the buffer is consistent.
    commitCurrentLine();

    // Find start and end byte
    const auto start_byte = getByteOffset(line, column);
    const auto end_byte = getByteOffset(lineEnd, columnEnd);

    // We need to fill a BufferEdit struct: it starts at line, column, and the old
    // end point is at lineEnd, columnEnd.
    const auto edit = BufferEdit{
        .start_byte = start_byte,
        .old_end_byte = end_byte,
        .new_end_byte = start_byte,
        .start = {.line = line, .column = column},
        .old_end = {.line = lineEnd, .column = columnEnd},
        .new_end = {.line = line, .column = column}
    };

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

    m_longest_line.onEdit(*this, edit);
    return edit;
}

BufferEdit LineBuffer::clear() {
    // Make sure the current line is folded back so the sizes below are exact.
    commitCurrentLine();

    const auto last_line = static_cast<uint32_t>(m_line_data.size()) - 1;
    const auto last_column = m_line_data.back().count;
    const auto buffer_size = getByteOffset(last_line, last_column);

    m_buffer.clear();
    m_current_line.clear();
    m_line_data.clear();
    m_line_data.push_back(LineData{.start = 0, .count = 0});
    m_current_line_index = 0;
    m_longest_line.reset();

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