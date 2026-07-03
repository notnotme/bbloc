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
    [[nodiscard]] BufferEdit insert(uint32_t line, uint32_t column, std::u16string_view characters) override;
    [[nodiscard]] BufferEdit erase(uint32_t line, uint32_t column, uint32_t lineEnd, uint32_t columnEnd) override;
    [[nodiscard]] BufferEdit clear() override;
};


#endif //VECTOR_BUFFER_H
