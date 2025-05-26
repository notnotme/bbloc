#ifndef STRING_BUFFER_H
#define STRING_BUFFER_H

#include <vector>
#include <string>
#include <string_view>

#include "TextBuffer.h"
#include "BufferEdit.h"


/**
 * @brief A text buffer implementation using a single contiguous UTF-16 string.
 *
 * This buffer tracks lines via indices (offset and length) into a single string.
 * This is not a gap buffer. I guess the performance is poor.
 */
class StringBuffer final : public TextBuffer {
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

    /** Internal contiguous buffer storing all characters. */
    std::u16string m_buffer;

    /** Metadata describing each line's position and length. */
    std::vector<LineData> m_line_data;

public:
    /** @brief Constructs an empty StringBuffer. */
    explicit StringBuffer();

    [[nodiscard]] std::u16string_view getString(uint32_t line) const override;
    [[nodiscard]] uint32_t getStringCount() const override;
    [[nodiscard]] uint32_t getByteOffset(uint32_t line, uint32_t column) const override;
    [[nodiscard]] uint32_t getByteCount(uint32_t lineStart, uint32_t columnStart, uint32_t lineEnd, uint32_t columnEnd) const override;
    [[nodiscard]] BufferEdit insert(uint32_t line, uint32_t column, std::u16string_view characters) override;
    [[nodiscard]] BufferEdit erase(uint32_t line, uint32_t column, uint32_t lineEnd, uint32_t columnEnd) override;
    [[nodiscard]] BufferEdit clear() override;

};


#endif //STRING_BUFFER_H
