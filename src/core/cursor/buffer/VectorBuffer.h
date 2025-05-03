#ifndef VECTOR_BUFFER_H
#define VECTOR_BUFFER_H

#include <vector>
#include <string>
#include <string_view>

#include "TextBuffer.h"


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

    [[nodiscard]] std::u16string_view getString(int32_t line) const override;
    [[nodiscard]] int32_t getStringCount() const override;
    void insert(int32_t& line, int32_t& column, std::u16string_view characters) override;
    void erase(int32_t& line, int32_t& column, int32_t lineEnd, int32_t columnEnd) override;
    void clear() override;
};


#endif //VECTOR_BUFFER_H
