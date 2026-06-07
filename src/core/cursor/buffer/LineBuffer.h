#ifndef LINE_BUFFER_H
#define LINE_BUFFER_H

#include <vector>
#include <string>
#include <string_view>

#include "TextBuffer.h"
#include "BufferEdit.h"


/**
 * @brief A text buffer implementation based on StringBuffer, but with a separate, smaller buffer for the current line.
 *
 * Like StringBuffer, all the text lives in a single contiguous UTF-16 buffer and
 * lines are tracked via indices (start offset and character count).
 *
 * The difference is the current line: the characters of the line currently being edited are extracted out of the full
 * text buffer.
 *
 * The current line is NOT a gap buffer, just a flat string. Because we would need to move the gap at the end of the
 * buffer before each rendering frame anyway (because the renderer expects contiguous data).
 *
 * While a line is the current line:
 *   - its characters are absent from m_buffer,
 *   - m_line_data[m_current].count is kept at 0,
 *   - m_line_data[m_current].start does not change (important to keep bytecount/offset for BufferEdits)
 *   - the source of truth for its content/length is m_current_line.
 *
 * Editing/erasing that stays on the current line only touches m_current_line and is cheap (no reflow of buffer).
 * The buffer is only modified when an edit happens on another line, at which point the current line is committed back
 * into the buffer and the current line is changed.
 */
class LineBuffer final : public TextBuffer {
private:
    /**
     * @brief Metadata for each line in the buffer.
     *
     * Represents a line by its starting index and length within the buffer.
     */
    struct LineData final {
        uint32_t start;   ///< Starting offset of the line in m_buffer.
        uint32_t count;   ///< Number of characters in the line.
    };

private:
    /** Contiguous buffer storing every line except the current line. */
    std::u16string m_buffer;

    /** Copy of the current line's characters. */
    std::u16string m_current_line;

    /** Index of the line. */
    uint32_t m_current_line_index;

    /** Metadata describing each line's position and length. */
    std::vector<LineData> m_line_data;

private:
    /**
     * @brief Commits m_current_line back into m_buffer at the current line slot.
     *
     * Inserts m_current_line into m_buffer, restores
     * m_line_data[m_current].count and shifts the following lines' offsets.
     */
    void commitCurrentLine();

    /** @return The real character count of a line (using m_current_line for the current one). */
    [[nodiscard]] uint32_t lineLength(uint32_t line) const;

public:
    /** @brief Constructs an empty LineBuffer. */
    explicit LineBuffer();

    [[nodiscard]] std::u16string_view getString(uint32_t line) const override;
    [[nodiscard]] uint32_t getStringCount() const override;
    [[nodiscard]] uint32_t getByteOffset(uint32_t line, uint32_t column) const override;
    [[nodiscard]] uint32_t getByteCount(uint32_t lineStart, uint32_t columnStart, uint32_t lineEnd, uint32_t columnEnd) const override;
    [[nodiscard]] BufferEdit insert(uint32_t line, uint32_t column, std::u16string_view characters) override;
    [[nodiscard]] BufferEdit erase(uint32_t line, uint32_t column, uint32_t lineEnd, uint32_t columnEnd) override;
    [[nodiscard]] BufferEdit clear() override;
};


#endif //LINE_BUFFER_H
