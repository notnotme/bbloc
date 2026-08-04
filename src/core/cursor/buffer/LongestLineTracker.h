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
#ifndef LONGEST_LINE_TRACKER_H
#define LONGEST_LINE_TRACKER_H

#include <cstdint>
#include <string_view>
#include <vector>

#include "BufferEdit.h"
#include "TextBuffer.h"


/**
 * @brief Incrementally tracks the longest line of a TextBuffer.
 *
 * Keeps a per-line metric (character count and tab count) updated from each BufferEdit,
 * so the longest weighted line length is available without scanning the buffer.
 * A full metric rescan (never a character scan) only happens when the longest line
 * shrank or when the tab weight changed.
 */
class LongestLineTracker final {
private:
    /** @brief Length metric of a single line. */
    struct LineMetric final {
        uint32_t count;     ///< Number of characters in the line.
        uint32_t tab_count; ///< Number of tab characters in the line.
    };

private:
    /** Metric of every line in the tracked buffer. */
    std::vector<LineMetric> m_metrics;

    /** Index of the longest line. */
    mutable uint32_t m_max_line;

    /** Weighted length of the longest line, using m_tab_weight. */
    mutable uint32_t m_max_length;

    /** Tab weight used for the current maximum. */
    mutable uint32_t m_tab_weight;

    /** true when the maximum must be recomputed from the metrics. */
    mutable bool m_is_max_dirty;

private:
    /** @brief Measures the character and tab count of a line. */
    [[nodiscard]] static LineMetric measureLine(std::u16string_view line);

    /** @return The length of a line where each tab weighs m_tab_weight characters. */
    [[nodiscard]] uint32_t weightedLength(const LineMetric &metric) const;

    /** @brief Recomputes the longest line from the per-line metrics. */
    void rescan() const;

public:
    /** @brief Deleted copy constructor. */
    LongestLineTracker(const LongestLineTracker &) = delete;

    /** @brief Deleted copy assignment operator. */
    LongestLineTracker &operator=(const LongestLineTracker &) = delete;

    /** @brief Constructs a tracker for a buffer holding a single empty line. */
    explicit LongestLineTracker();

    /** @brief Resets the tracker to a single empty line. */
    void reset();

    /**
     * @brief Updates the per-line metrics and the tracked maximum after a buffer edit.
     *
     * Must be called after the buffer applied the edit, so the touched lines can be re-read.
     *
     * @param buffer The edited buffer.
     * @param edit Object describing the modified position ranges.
     */
    void onEdit(const TextBuffer &buffer, const BufferEdit &edit);

    /**
     * @brief Returns the weighted character length of the longest line.
     *
     * Tabs weigh tabWeight characters, every other character weighs one.
     *
     * @param tabWeight The number of character widths a tab character occupies.
     * @return The weighted length of the longest line, in characters.
     */
    [[nodiscard]] uint32_t getLongestLineLength(uint32_t tabWeight) const;

    /**
     * @brief Returns the tab count of a single line.
     *
     * Read from the per-line metrics kept updated by onEdit, so no character scan happens.
     *
     * @param line The line index to query.
     * @return The number of tab characters in the line.
     */
    [[nodiscard]] uint32_t getLineTabCount(uint32_t line) const;
};


#endif //LONGEST_LINE_TRACKER_H
