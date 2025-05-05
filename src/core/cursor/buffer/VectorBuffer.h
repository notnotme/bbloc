#ifndef VECTOR_BUFFER_H
#define VECTOR_BUFFER_H

#include <vector>
#include <string>
#include <string_view>

#include "TextBuffer.h"
#include "BufferEdit.h"


/**
 * @brief Implementation of TextBuffer using a std::vector.
 *
 * Stores text as a vector of UTF-16 lines. Lines are not gap buffers.
 */
class VectorBuffer final : public TextBuffer {
private:
    /** Holds the lines of text as UTF-16 strings. */
    std::vector<std::u16string> m_lines;

public:
    /** @brief Constructs an empty VectorBuffer. */
    explicit VectorBuffer();

    [[nodiscard]] std::u16string_view getString(uint32_t line) const override;
    [[nodiscard]] uint32_t getStringCount() const override;
    [[nodiscard]] uint32_t getByteOffset(uint32_t line, uint32_t column) const override;
    [[nodiscard]] uint32_t getByteCount(uint32_t lineStart, uint32_t columnStart, uint32_t lineEnd, uint32_t columnEnd) const override;
    BufferEdit insert(uint32_t& line, uint32_t& column, std::u16string_view characters) override;
    BufferEdit erase(uint32_t line, uint32_t column, uint32_t lineEnd, uint32_t columnEnd) override;
    BufferEdit clear() override;
};


#endif //VECTOR_BUFFER_H
