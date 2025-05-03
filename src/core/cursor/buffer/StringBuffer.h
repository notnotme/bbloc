#ifndef STRING_BUFFER_H
#define STRING_BUFFER_H

#include <vector>
#include <string>
#include <string_view>

#include "TextBuffer.h"


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
     * Represents a line by its starting index and length within the buffer.
     */
    struct LineData final {
        int32_t start;   ///< Starting offset of the line in m_buffer.
        int32_t count;   ///< Number of characters in the line.
    };

    /** Internal contiguous buffer storing all characters. */
    std::u16string m_buffer;

    /** Metadata describing each line's position and length. */
    std::vector<LineData> m_line_data;

public:
    /** @brief Constructs an empty StringBuffer. */
    explicit StringBuffer();

    [[nodiscard]] std::u16string_view getString(int32_t line) const override;
    [[nodiscard]] int32_t getStringCount() const override;
    void insert(int32_t& line, int32_t& column, std::u16string_view characters) override;
    void erase(int32_t& line, int32_t& column, int32_t lineEnd, int32_t columnEnd) override;
    void clear() override;
};


#endif //STRING_BUFFER_H
