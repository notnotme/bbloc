#ifndef TEXT_RANGE_H
#define TEXT_RANGE_H


/** @brief Represents a range of text by line, column position. */
struct TextRange {
    uint32_t line_start;     ///< The line where the range starts
    uint32_t column_start;   ///< The column where the range starts
    uint32_t line_end;       ///< The line where the range ends
    uint32_t column_end;     ///< The column where the range ends
};


#endif //TEXT_RANGE_H
